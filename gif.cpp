#include "gif.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

namespace gif
{

#define MIN(A, B) ((A) < (B) ? (A) : (B))
#define MAX(A, B) ((A) > (B) ? (A) : (B))

    typedef struct Entry {
        uint16_t length;
        uint16_t prefix;
        uint8_t  suffix;
    } Entry;

    typedef struct Table {
        int bulk;
        int nentries;
        Entry* entries;
    } Table;

    static uint16_t
        read_num(int fd)
    {
        uint8_t bytes[2];

        _read(fd, bytes, 2);
        return bytes[0] + (((uint16_t)bytes[1]) << 8);
    }

    gd_GIF*
        gd_open_gif(const char* fname)
    {
        int fd;
        uint8_t sigver[3];
        uint16_t width, height, depth;
        uint8_t fdsz, bgidx, aspect;
        int i;
        uint8_t* bgcolor;
        int gct_sz;
        gd_GIF* gif = nullptr;

        fd = _open(fname, O_RDONLY);
        if (fd == -1) return NULL;
#ifdef _WIN32
        _setmode(fd, O_BINARY);
#endif
        /* Header */
        _read(fd, sigver, 3);
        if (memcmp(sigver, "GIF", 3) != 0) {
            fprintf(stderr, "invalid signature\n");
            free(gif);
            _close(fd);
            return 0;
        }
        /* Version */
        _read(fd, sigver, 3);
        if (memcmp(sigver, "89a", 3) != 0) {
            fprintf(stderr, "invalid version\n");
            free(gif);
            _close(fd);
            return 0;
        }
        /* Width x Height */
        width = read_num(fd);
        height = read_num(fd);
        /* FDSZ */
        _read(fd, &fdsz, 1);
        /* Presence of GCT */
        if (!(fdsz & 0x80)) {
            fprintf(stderr, "no global color table\n");
            free(gif);
            _close(fd);
            return 0;
        }
        /* Color Space's Depth */
        depth = ((fdsz >> 4) & 7) + 1;
        /* Ignore Sort Flag. */
        /* GCT Size */
        gct_sz = 1 << ((fdsz & 0x07) + 1);
        /* Background Color Index */
        _read(fd, &bgidx, 1);
        /* Aspect Ratio */
        _read(fd, &aspect, 1);
        /* Create gd_GIF Structure. */
        gif = (gd_GIF*)calloc(1, sizeof(*gif));
        if (!gif) {
            free(gif);
            _close(fd);
            return 0;
        }
        gif->fd = fd;
        gif->width = width;
        gif->height = height;
        gif->depth = depth;
        /* Read GCT */
        gif->gct.size = gct_sz;
        _read(fd, gif->gct.colors, 3 * gif->gct.size);
        gif->palette = &gif->gct;
        gif->bgindex = bgidx;
        gif->frame = (uint8_t*)calloc(4, width * height);
        if (!gif->frame) {
            free(gif);
            _close(fd);
            return 0;
        }
        gif->canvas = &gif->frame[width * height];
        if (gif->bgindex)
            memset(gif->frame, gif->bgindex, gif->width * gif->height);
        bgcolor = &gif->palette->colors[gif->bgindex * 3];
        if (bgcolor[0] || bgcolor[1] || bgcolor[2])
            for (i = 0; i < gif->width * gif->height; i++)
                memcpy(&gif->canvas[i * 3], bgcolor, 3);
        gif->anim_start = _lseek(fd, 0, SEEK_CUR);
        return gif;
    }

    static void
        discard_sub_blocks(gd_GIF* gif)
    {
        uint8_t size;

        do {
            _read(gif->fd, &size, 1);
            _lseek(gif->fd, size, SEEK_CUR);
        } while (size);
    }

    static void
        read_plain_text_ext(gd_GIF* gif)
    {
        if (gif->plain_text) {
            uint16_t tx, ty, tw, th;
            uint8_t cw, ch, fg, bg;
            off_t sub_block;
            _lseek(gif->fd, 1, SEEK_CUR); /* block size = 12 */
            tx = read_num(gif->fd);
            ty = read_num(gif->fd);
            tw = read_num(gif->fd);
            th = read_num(gif->fd);
            _read(gif->fd, &cw, 1);
            _read(gif->fd, &ch, 1);
            _read(gif->fd, &fg, 1);
            _read(gif->fd, &bg, 1);
            sub_block = _lseek(gif->fd, 0, SEEK_CUR);
            gif->plain_text(gif, tx, ty, tw, th, cw, ch, fg, bg);
            _lseek(gif->fd, sub_block, SEEK_SET);
        }
        else {
            /* Discard plain text metadata. */
            _lseek(gif->fd, 13, SEEK_CUR);
        }
        /* Discard plain text sub-blocks. */
        discard_sub_blocks(gif);
    }

    static void
        read_graphic_control_ext(gd_GIF* gif)
    {
        uint8_t rdit;

        /* Discard block size (always 0x04). */
        _lseek(gif->fd, 1, SEEK_CUR);
        _read(gif->fd, &rdit, 1);
        gif->gce.disposal = (rdit >> 2) & 3;
        gif->gce.input = rdit & 2;
        gif->gce.transparency = rdit & 1;
        gif->gce.delay = read_num(gif->fd);
        _read(gif->fd, &gif->gce.tindex, 1);
        /* Skip block terminator. */
        _lseek(gif->fd, 1, SEEK_CUR);
    }

    static void
        read_comment_ext(gd_GIF* gif)
    {
        if (gif->comment) {
            off_t sub_block = _lseek(gif->fd, 0, SEEK_CUR);
            gif->comment(gif);
            _lseek(gif->fd, sub_block, SEEK_SET);
        }
        /* Discard comment sub-blocks. */
        discard_sub_blocks(gif);
    }

    static void
        read_application_ext(gd_GIF* gif)
    {
        char app_id[8];
        char app_auth_code[3];

        /* Discard block size (always 0x0B). */
        _lseek(gif->fd, 1, SEEK_CUR);
        /* Application Identifier. */
        _read(gif->fd, app_id, 8);
        /* Application Authentication Code. */
        _read(gif->fd, app_auth_code, 3);
        if (!strncmp(app_id, "NETSCAPE", sizeof(app_id))) {
            /* Discard block size (0x03) and constant byte (0x01). */
            _lseek(gif->fd, 2, SEEK_CUR);
            gif->loop_count = read_num(gif->fd);
            /* Skip block terminator. */
            _lseek(gif->fd, 1, SEEK_CUR);
        }
        else if (gif->application) {
            off_t sub_block = _lseek(gif->fd, 0, SEEK_CUR);
            gif->application(gif, app_id, app_auth_code);
            _lseek(gif->fd, sub_block, SEEK_SET);
            discard_sub_blocks(gif);
        }
        else {
            discard_sub_blocks(gif);
        }
    }

    static void
        read_ext(gd_GIF* gif)
    {
        uint8_t label;

        _read(gif->fd, &label, 1);
        switch (label) {
        case 0x01:
            read_plain_text_ext(gif);
            break;
        case 0xF9:
            read_graphic_control_ext(gif);
            break;
        case 0xFE:
            read_comment_ext(gif);
            break;
        case 0xFF:
            read_application_ext(gif);
            break;
        default:
            fprintf(stderr, "unknown extension: %02X\n", label);
        }
    }

    static Table*
        new_table(int key_size)
    {
        int key;
        int init_bulk = MAX(1 << (key_size + 1), 0x100);
        Table* table = (gif::Table*)malloc(sizeof(*table) + sizeof(Entry) * init_bulk);
        if (table) {
            table->bulk = init_bulk;
            table->nentries = (1 << key_size) + 2;
            table->entries = (Entry*)&table[1];
            for (key = 0; key < (1 << key_size); key++)
                table->entries[key] = Entry(1, 0xFFF, key);
        }
        return table;
    }

    /* Add table entry. Return value:
     *  0 on success
     *  +1 if key size must be incremented after this addition
     *  -1 if could not realloc table */
    static int
        add_entry(Table** tablep, uint16_t length, uint16_t prefix, uint8_t suffix)
    {
        Table* table = *tablep;
        if (table->nentries == table->bulk) {
            table->bulk *= 2;
            table = (gif::Table*)realloc(table, sizeof(*table) + sizeof(Entry) * table->bulk);
            if (!table) return -1;
            table->entries = (Entry*)&table[1];
            *tablep = table;
        }
        table->entries[table->nentries] = Entry(length, prefix, suffix);
        table->nentries++;
        if ((table->nentries & (table->nentries - 1)) == 0)
            return 1;
        return 0;
    }

    static uint16_t
        get_key(gd_GIF* gif, int key_size, uint8_t* sub_len, uint8_t* shift, uint8_t* byte)
    {
        int bits_read;
        int rpad;
        int frag_size;
        uint16_t key;

        key = 0;
        for (bits_read = 0; bits_read < key_size; bits_read += frag_size) {
            rpad = (*shift + bits_read) % 8;
            if (rpad == 0) {
                /* Update byte. */
                if (*sub_len == 0) {
                    _read(gif->fd, sub_len, 1); /* Must be nonzero! */
                    if (*sub_len == 0)
                        return 0x1000;
                }
                _read(gif->fd, byte, 1);
                (*sub_len)--;
            }
            frag_size = MIN(key_size - bits_read, 8 - rpad);
            key |= ((uint16_t)((*byte) >> rpad)) << bits_read;
        }
        /* Clear extra bits to the left. */
        key &= (1 << key_size) - 1;
        *shift = (*shift + key_size) % 8;
        return key;
    }

    /* Compute output index of y-th input line, in frame of height h. */
    static int
        interlaced_line_index(int h, int y)
    {
        int p; /* number of lines in current pass */

        p = (h - 1) / 8 + 1;
        if (y < p) /* pass 1 */
            return y * 8;
        y -= p;
        p = (h - 5) / 8 + 1;
        if (y < p) /* pass 2 */
            return y * 8 + 4;
        y -= p;
        p = (h - 3) / 4 + 1;
        if (y < p) /* pass 3 */
            return y * 4 + 2;
        y -= p;
        /* pass 4 */
        return y * 2 + 1;
    }

    /* Decompress image pixels.
     * Return 0 on success or -1 on out-of-memory (w.r.t. LZW code table). */
    static int
        read_image_data(gd_GIF* gif, int interlace)
    {
        uint8_t sub_len, shift, byte;
        int init_key_size, key_size, table_is_full;
        int frm_off, frm_size, str_len, i, p, x, y;
        uint16_t key, clear, stop;
        int ret;
        Table* table;
        Entry entry;
        off_t start, end;

        _read(gif->fd, &byte, 1);
        key_size = (int)byte;
        if (key_size < 2 || key_size > 8)
            return -1;

        start = _lseek(gif->fd, 0, SEEK_CUR);
        discard_sub_blocks(gif);
        end = _lseek(gif->fd, 0, SEEK_CUR);
        _lseek(gif->fd, start, SEEK_SET);
        clear = 1 << key_size;
        stop = clear + 1;
        table = new_table(key_size);
        key_size++;
        init_key_size = key_size;
        sub_len = shift = 0;
        key = get_key(gif, key_size, &sub_len, &shift, &byte); /* clear code */
        frm_off = 0;
        ret = 0;
        frm_size = gif->fw * gif->fh;
        while (frm_off < frm_size) {
            if (key == clear) {
                key_size = init_key_size;
                table->nentries = (1 << (key_size - 1)) + 2;
                table_is_full = 0;
            }
            else if (!table_is_full) {
                ret = add_entry(&table, str_len + 1, key, entry.suffix);
                if (ret == -1) {
                    free(table);
                    return -1;
                }
                if (table->nentries == 0x1000) {
                    ret = 0;
                    table_is_full = 1;
                }
            }
            key = get_key(gif, key_size, &sub_len, &shift, &byte);
            if (key == clear) continue;
            if (key == stop || key == 0x1000) break;
            if (ret == 1) key_size++;
            entry = table->entries[key];
            str_len = entry.length;
            for (i = 0; i < str_len; i++) {
                p = frm_off + entry.length - 1;
                x = p % gif->fw;
                y = p / gif->fw;
                if (interlace)
                    y = interlaced_line_index((int)gif->fh, y);
                gif->frame[(gif->fy + y) * gif->width + gif->fx + x] = entry.suffix;
                if (entry.prefix == 0xFFF)
                    break;
                else
                    entry = table->entries[entry.prefix];
            }
            frm_off += str_len;
            if (key < table->nentries - 1 && !table_is_full)
                table->entries[table->nentries - 1].suffix = entry.suffix;
        }
        free(table);
        if (key == stop)
            _read(gif->fd, &sub_len, 1); /* Must be zero! */
        _lseek(gif->fd, end, SEEK_SET);
        return 0;
    }

    /* Read image.
     * Return 0 on success or -1 on out-of-memory (w.r.t. LZW code table). */
    static int
        read_image(gd_GIF* gif)
    {
        uint8_t fisrz;
        int interlace;

        /* Image Descriptor. */
        gif->fx = read_num(gif->fd);
        gif->fy = read_num(gif->fd);

        if (gif->fx >= gif->width || gif->fy >= gif->height)
            return -1;

        gif->fw = read_num(gif->fd);
        gif->fh = read_num(gif->fd);

        gif->fw = MIN(gif->fw, gif->width - gif->fx);
        gif->fh = MIN(gif->fh, gif->height - gif->fy);

        _read(gif->fd, &fisrz, 1);
        interlace = fisrz & 0x40;
        /* Ignore Sort Flag. */
        /* Local Color Table? */
        if (fisrz & 0x80) {
            /* Read LCT */
            gif->lct.size = 1 << ((fisrz & 0x07) + 1);
            _read(gif->fd, gif->lct.colors, 3 * gif->lct.size);
            gif->palette = &gif->lct;
        }
        else
            gif->palette = &gif->gct;
        /* Image Data. */
        return read_image_data(gif, interlace);
    }

    static void
        render_frame_rect(gd_GIF* gif, uint8_t* buffer)
    {
        int i, j, k;
        uint8_t index, * color;
        i = gif->fy * gif->width + gif->fx;
        for (j = 0; j < gif->fh; j++) {
            for (k = 0; k < gif->fw; k++) {
                index = gif->frame[(gif->fy + j) * gif->width + gif->fx + k];
                color = &gif->palette->colors[index * 3];
                if (!gif->gce.transparency || index != gif->gce.tindex)
                    memcpy(&buffer[(i + k) * 3], color, 3);
            }
            i += gif->width;
        }
    }

    static void
        dispose(gd_GIF* gif)
    {
        int i, j, k;
        uint8_t* bgcolor;
        switch (gif->gce.disposal) {
        case 2: /* Restore to background color. */
            bgcolor = &gif->palette->colors[gif->bgindex * 3];
            i = gif->fy * gif->width + gif->fx;
            for (j = 0; j < gif->fh; j++) {
                for (k = 0; k < gif->fw; k++)
                    memcpy(&gif->canvas[(i + k) * 3], bgcolor, 3);
                i += gif->width;
            }
            break;
        case 3: /* Restore to previous, i.e., don't update canvas.*/
            break;
        default:
            /* Add frame non-transparent pixels to canvas. */
            render_frame_rect(gif, gif->canvas);
        }
    }

    /* Return 1 if got a frame; 0 if got GIF trailer; -1 if error. */
    int
        gd_get_frame(gd_GIF* gif)
    {
        char sep;

        dispose(gif);
        _read(gif->fd, &sep, 1);
        while (sep != ',') {
            if (sep == ';')
                return 0;
            if (sep == '!')
                read_ext(gif);
            else return -1;
            _read(gif->fd, &sep, 1);
        }
        if (read_image(gif) == -1)
            return -1;
        return 1;
    }

    void
        gd_render_frame(gd_GIF* gif, uint8_t* buffer)
    {
        memcpy(buffer, gif->canvas, gif->width * gif->height * 3);
        render_frame_rect(gif, buffer);
    }

    int
        gd_is_bgcolor(gd_GIF* gif, uint8_t color[3])
    {
        return !memcmp(&gif->palette->colors[gif->bgindex * 3], color, 3);
    }

    void
        gd_rewind(gd_GIF* gif)
    {
        _lseek(gif->fd, gif->anim_start, SEEK_SET);
    }

    void
        gd_close_gif(gd_GIF* gif)
    {
        _close(gif->fd);
        free(gif->frame);
        free(gif);
    }


    //============================================================================================

#define MULU16(a, b) (((uint32_t)a) * ((uint32_t)b)) // helper macro to correctly multiply two U16's without default signed int promotion
#define SIZE_FRAME_QUEUE (3)

// CGIF_Frame type
// note: internal sections, subject to change in future versions
    typedef struct {
        CGIF_FrameConfig config;
        uint8_t          disposalMethod;
        uint8_t          transIndex;
    } CGIF_Frame;

    // CGIF type
    // note: internal sections, subject to change in future versions
    struct st_gif {
        CGIF_Frame* aFrames[SIZE_FRAME_QUEUE]; // (internal) we need to keep the last three frames in memory.
        CGIF_Config        config;                    // (internal) configuration parameters of the GIF
        CGIFRaw* pGIFRaw;                   // (internal) raw GIF stream
        FILE* pFile;
        cgif_result        curResult;
        int                iHEAD;                     // (internal) index to current HEAD frame in aFrames queue
    };

    // dimension result type
    typedef struct {
        uint16_t width;
        uint16_t height;
        uint16_t top;
        uint16_t left;
    } DimResult;

    /* write callback. returns 0 on success or -1 on error.  */
    static int writecb(void* pContext, const uint8_t* pData, const size_t numBytes) {
        CGIF* pGIF;
        size_t r;

        pGIF = (CGIF*)pContext;
        if (pGIF->pFile) {
            r = fwrite(pData, 1, numBytes, pGIF->pFile);
            if (r == numBytes) return 0;
            else return -1;
        }
        else if (pGIF->config.pWriteFn) {
            return pGIF->config.pWriteFn(pGIF->config.pContext, pData, numBytes);
        }
        return 0;
    }

    /* free space allocated for CGIF struct */
    static void freeCGIF(CGIF* pGIF) {
        if ((pGIF->config.attrFlags & CGIF_ATTR_NO_GLOBAL_TABLE) == 0) {
            free(pGIF->config.pGlobalPalette);
        }
        free(pGIF);
    }

    /* create a new GIF */
    CGIF* cgif_newgif(CGIF_Config* pConfig) {
        FILE* pFile;
        CGIF* pGIF;
        CGIFRaw* pGIFRaw; // raw GIF stream
        CGIFRaw_Config rawConfig = { 0 };
        // width or heigth cannot be zero
        if (!pConfig->width || !pConfig->height) {
            return NULL;
        }
        pFile = NULL;
        // open output file (if necessary)
        if (pConfig->path) {
            pFile = fopen(pConfig->path, "wb");
            if (pFile == NULL) {
                return NULL; // error: fopen failed
            }
        }
        // allocate space for CGIF context
        pGIF = (gif::CGIF*)malloc(sizeof(CGIF));
        if (pGIF == NULL) {
            if (pFile) {
                fclose(pFile);
            }
            return NULL; // error -> malloc failed
        }

        memset(pGIF, 0, sizeof(CGIF));
        pGIF->pFile = pFile;
        pGIF->iHEAD = 1;
        memcpy(&(pGIF->config), pConfig, sizeof(CGIF_Config));
        // make a deep copy of global color tabele (GCT), if required.
        if ((pConfig->attrFlags & CGIF_ATTR_NO_GLOBAL_TABLE) == 0) {
            pGIF->config.pGlobalPalette = (uint8_t*)malloc(pConfig->numGlobalPaletteEntries * 3);
            memcpy(pGIF->config.pGlobalPalette, pConfig->pGlobalPalette, pConfig->numGlobalPaletteEntries * 3);
        }

        rawConfig.pGCT = pConfig->pGlobalPalette;
        rawConfig.sizeGCT = (pConfig->attrFlags & CGIF_ATTR_NO_GLOBAL_TABLE) ? 0 : pConfig->numGlobalPaletteEntries;
        // translate CGIF_ATTR_* to CGIF_RAW_ATTR_* flags
        rawConfig.attrFlags = (pConfig->attrFlags & CGIF_ATTR_IS_ANIMATED) ? CGIF_RAW_ATTR_IS_ANIMATED : 0;
        rawConfig.attrFlags |= (pConfig->attrFlags & CGIF_ATTR_NO_LOOP) ? CGIF_RAW_ATTR_NO_LOOP : 0;
        rawConfig.width = pConfig->width;
        rawConfig.height = pConfig->height;
        rawConfig.numLoops = pConfig->numLoops;
        rawConfig.pWriteFn = writecb;
        rawConfig.pContext = (void*)pGIF;
        // pass config down and create a new raw GIF stream.
        pGIFRaw = cgif_raw_newgif(&rawConfig);
        // check for errors
        if (pGIFRaw == NULL) {
            if (pFile) {
                fclose(pFile);
            }
            freeCGIF(pGIF);
            return NULL;
        }

        pGIF->pGIFRaw = pGIFRaw;
        // assume error per default.
        // set to CGIF_OK by the first successful cgif_addframe() call, as a GIF without frames is invalid.
        pGIF->curResult = CGIF_PENDING;
        return pGIF;
    }

    /* compare given pixel indices using the correct local or global color table; returns 0 if the two pixels are RGB equal */
    static int cmpPixel(const CGIF* pGIF, const CGIF_FrameConfig* pCur, const CGIF_FrameConfig* pBef, const uint8_t iCur, const uint8_t iBef) {
        uint8_t* pBefCT; // color table to use for pBef
        uint8_t* pCurCT; // color table to use for pCur

        if ((pCur->attrFlags & CGIF_FRAME_ATTR_HAS_SET_TRANS) && iCur == pCur->transIndex) {
            return 0; // identical
        }
        if ((pBef->attrFlags & CGIF_FRAME_ATTR_HAS_SET_TRANS) && iBef == pBef->transIndex) {
            return 1; // done: cannot compare
        }
        // safety bounds check
        const uint16_t sizeCTBef = (pBef->attrFlags & CGIF_FRAME_ATTR_USE_LOCAL_TABLE) ? pBef->numLocalPaletteEntries : pGIF->config.numGlobalPaletteEntries;
        const uint16_t sizeCTCur = (pCur->attrFlags & CGIF_FRAME_ATTR_USE_LOCAL_TABLE) ? pCur->numLocalPaletteEntries : pGIF->config.numGlobalPaletteEntries;
        if ((iBef >= sizeCTBef) || (iCur >= sizeCTCur)) {
            return 1; // error: out-of-bounds - cannot compare
        }
        pBefCT = (pBef->attrFlags & CGIF_FRAME_ATTR_USE_LOCAL_TABLE) ? pBef->pLocalPalette : pGIF->config.pGlobalPalette; // local or global table used?
        pCurCT = (pCur->attrFlags & CGIF_FRAME_ATTR_USE_LOCAL_TABLE) ? pCur->pLocalPalette : pGIF->config.pGlobalPalette; // local or global table used?
        return memcmp(pBefCT + iBef * 3, pCurCT + iCur * 3, 3);
    }

    /* optimize GIF file size by only redrawing the rectangular area that differs from previous frame */
    static uint8_t* doWidthHeightOptim(CGIF* pGIF, CGIF_FrameConfig* pCur, CGIF_FrameConfig* pBef, DimResult* pResult) {
        uint8_t* pNewImageData;
        const uint8_t* pCurImageData;
        const uint8_t* pBefImageData;
        uint16_t       i, x;
        uint16_t       newHeight, newWidth, newLeft, newTop;
        const uint16_t width = pGIF->config.width;
        const uint16_t height = pGIF->config.height;
        uint8_t        iCur, iBef;

        pCurImageData = pCur->pImageData;
        pBefImageData = pBef->pImageData;
        // find top
        i = 0;
        while (i < height) {
            for (int c = 0; c < width; ++c) {
                iCur = *(pCurImageData + MULU16(i, width) + c);
                iBef = *(pBefImageData + MULU16(i, width) + c);
                if (cmpPixel(pGIF, pCur, pBef, iCur, iBef) != 0) {
                    goto FoundTop;
                }
            }
            ++i;
        }
    FoundTop:
        if (i == height) { // need dummy pixel (frame is identical with one before)
            // TBD we might make it possible to merge identical frames in the future
            newWidth = 1;
            newHeight = 1;
            newLeft = 0;
            newTop = 0;
            goto Done;
        }
        newTop = i;

        // find actual height
        i = height - 1;
        while (i > newTop) {
            for (int c = 0; c < width; ++c) {
                iCur = *(pCurImageData + MULU16(i, width) + c);
                iBef = *(pBefImageData + MULU16(i, width) + c);
                if (cmpPixel(pGIF, pCur, pBef, iCur, iBef) != 0) {
                    goto FoundHeight;
                }
            }
            --i;
        }
    FoundHeight:
        newHeight = (i + 1) - newTop;

        // find left
        i = newTop;
        x = 0;
        while (cmpPixel(pGIF, pCur, pBef, pCurImageData[MULU16(i, width) + x], pBefImageData[MULU16(i, width) + x]) == 0) {
            ++i;
            if (i > (newTop + newHeight - 1)) {
                ++x; //(x==width cannot happen as goto Done is triggered in the only possible case before)
                i = newTop;
            }
        }
        newLeft = x;

        // find actual width
        i = newTop;
        x = width - 1;
        while (cmpPixel(pGIF, pCur, pBef, pCurImageData[MULU16(i, width) + x], pBefImageData[MULU16(i, width) + x]) == 0) {
            ++i;
            if (i > (newTop + newHeight - 1)) {
                --x; //(x<newLeft cannot happen as goto Done is triggered in the only possible case before)
                i = newTop;
            }
        }
        newWidth = (x + 1) - newLeft;

    Done:

        // create new image data
        pNewImageData = (uint8_t*)malloc(MULU16(newWidth, newHeight)); // TBD check return value of malloc
        for (i = 0; i < newHeight; ++i) {
            memcpy(pNewImageData + MULU16(i, newWidth), pCurImageData + MULU16((i + newTop), width) + newLeft, newWidth);
        }

        // set new width, height, top, left in DimResult struct
        pResult->width = newWidth;
        pResult->height = newHeight;
        pResult->top = newTop;
        pResult->left = newLeft;
        return pNewImageData;
    }

    /* move frame down to the raw GIF API */
    static cgif_result flushFrame(CGIF* pGIF, CGIF_Frame* pCur, CGIF_Frame* pBef) {
        CGIFRaw_FrameConfig rawConfig;
        DimResult           dimResult;
        uint8_t* pTmpImageData;
        uint8_t* pBefImageData;
        int                 isFirstFrame, useLCT, hasAlpha, hasSetTransp;
        uint16_t            numPaletteEntries;
        uint16_t            imageWidth, imageHeight, width, height, top, left;
        uint8_t             transIndex, disposalMethod;
        cgif_result         r;

        imageWidth = pGIF->config.width;
        imageHeight = pGIF->config.height;
        isFirstFrame = (pBef == NULL) ? 1 : 0;
        useLCT = (pCur->config.attrFlags & CGIF_FRAME_ATTR_USE_LOCAL_TABLE) ? 1 : 0; // LCT stands for "local color table"
        hasAlpha = ((pGIF->config.attrFlags & CGIF_ATTR_HAS_TRANSPARENCY) || (pCur->config.attrFlags & CGIF_FRAME_ATTR_HAS_ALPHA)) ? 1 : 0;
        hasSetTransp = (pCur->config.attrFlags & CGIF_FRAME_ATTR_HAS_SET_TRANS) ? 1 : 0;
        disposalMethod = pCur->disposalMethod;
        transIndex = pCur->transIndex;
        // deactivate impossible size optimizations
        //  => in case alpha channel is used
        // CGIF_FRAME_GEN_USE_TRANSPARENCY and CGIF_FRAME_GEN_USE_DIFF_WINDOW are not possible
        if (isFirstFrame || hasAlpha) {
            pCur->config.genFlags &= ~(CGIF_FRAME_GEN_USE_TRANSPARENCY | CGIF_FRAME_GEN_USE_DIFF_WINDOW);
        }
        // transparency setting (which areas are identical to the frame before) provided by user:
        // CGIF_FRAME_GEN_USE_TRANSPARENCY not possible
        if (hasSetTransp) {
            pCur->config.genFlags &= ~(CGIF_FRAME_GEN_USE_TRANSPARENCY);
        }
        numPaletteEntries = (useLCT) ? pCur->config.numLocalPaletteEntries : pGIF->config.numGlobalPaletteEntries;
        // switch off transparency optimization if color table is full (no free spot for the transparent index), TBD: count used colors, adapt table
        if (numPaletteEntries == 256) {
            pCur->config.genFlags &= ~CGIF_FRAME_GEN_USE_TRANSPARENCY;
        }

        // purge overlap of current frame and frame before (width - height optim), if required (CGIF_FRAME_GEN_USE_DIFF_WINDOW set)
        if (pCur->config.genFlags & CGIF_FRAME_GEN_USE_DIFF_WINDOW) {
            pTmpImageData = doWidthHeightOptim(pGIF, &pCur->config, &pBef->config, &dimResult);
            width = dimResult.width;
            height = dimResult.height;
            top = dimResult.top;
            left = dimResult.left;
        }
        else {
            pTmpImageData = NULL;
            width = imageWidth;
            height = imageHeight;
            top = 0;
            left = 0;
        }

        // mark matching areas of the previous frame as transparent, if required (CGIF_FRAME_GEN_USE_TRANSPARENCY set)
        if (pCur->config.genFlags & CGIF_FRAME_GEN_USE_TRANSPARENCY) {
            // set transIndex to next free index
            int pow2 = calcNextPower2Ex(numPaletteEntries);
            pow2 = (pow2 < 2) ? 2 : pow2; // TBD keep transparency index behavior as in V0.1.0 (for now)
            transIndex = (1 << pow2) - 1;
            if (transIndex < numPaletteEntries) {
                transIndex = (1 << (pow2 + 1)) - 1;
            }
            if (pTmpImageData == NULL) {
                pTmpImageData = (uint8_t*)malloc(MULU16(imageWidth, imageHeight)); // TBD check return value of malloc
                memcpy(pTmpImageData, pCur->config.pImageData, MULU16(imageWidth, imageHeight));
            }
            pBefImageData = pBef->config.pImageData;
            for (int i = 0; i < height; ++i) {
                for (int x = 0; x < width; ++x) {
                    if (cmpPixel(pGIF, &pCur->config, &pBef->config, pTmpImageData[MULU16(i, width) + x], pBefImageData[MULU16(top + i, imageWidth) + (left + x)]) == 0) {
                        pTmpImageData[MULU16(i, width) + x] = transIndex;
                    }
                }
            }
        }

        // move frame down to GIF raw API
        rawConfig.pLCT = pCur->config.pLocalPalette;
        rawConfig.pImageData = (pTmpImageData) ? pTmpImageData : pCur->config.pImageData;
        rawConfig.attrFlags = 0;
        if (hasAlpha || (pCur->config.genFlags & CGIF_FRAME_GEN_USE_TRANSPARENCY) || hasSetTransp) {
            rawConfig.attrFlags |= CGIF_RAW_FRAME_ATTR_HAS_TRANS;
        }
        rawConfig.attrFlags |= (pCur->config.attrFlags & CGIF_FRAME_ATTR_INTERLACED) ? CGIF_RAW_FRAME_ATTR_INTERLACED : 0;
        rawConfig.width = width;
        rawConfig.height = height;
        rawConfig.top = top;
        rawConfig.left = left;
        rawConfig.delay = pCur->config.delay;
        rawConfig.sizeLCT = (useLCT) ? pCur->config.numLocalPaletteEntries : 0;
        rawConfig.disposalMethod = disposalMethod;
        rawConfig.transIndex = transIndex;
        r = cgif_raw_addframe(pGIF->pGIFRaw, &rawConfig);
        free(pTmpImageData);
        return r;
    }

    static void freeFrame(CGIF_Frame* pFrame) {
        if (pFrame) {
            free(pFrame->config.pImageData);
            if (pFrame->config.attrFlags & CGIF_FRAME_ATTR_USE_LOCAL_TABLE) {
                free(pFrame->config.pLocalPalette);
            }
            free(pFrame);
        }
    }

    static void copyFrameConfig(CGIF_FrameConfig* pDest, CGIF_FrameConfig* pSrc) {
        pDest->pLocalPalette = pSrc->pLocalPalette; // might need a deep copy
        pDest->pImageData = pSrc->pImageData;    // might need a deep copy
        pDest->attrFlags = pSrc->attrFlags;
        pDest->genFlags = pSrc->genFlags;
        pDest->delay = pSrc->delay;
        pDest->numLocalPaletteEntries = pSrc->numLocalPaletteEntries;
        // copy transIndex if necessary (field added with V0.2.0; avoid binary incompatibility)
        if (pSrc->attrFlags & (CGIF_FRAME_ATTR_HAS_ALPHA | CGIF_FRAME_ATTR_HAS_SET_TRANS)) {
            pDest->transIndex = pSrc->transIndex;
        }
    }

    /* queue a new GIF frame */
    int cgif_addframe(CGIF* pGIF, CGIF_FrameConfig* pConfig) {
        CGIF_Frame* pNewFrame;
        int         hasAlpha, hasSetTransp;
        int         i;
        cgif_result r;

        // check for previous errors
        if (pGIF->curResult != CGIF_OK && pGIF->curResult != CGIF_PENDING) {
            return pGIF->curResult;
        }
        hasAlpha = ((pGIF->config.attrFlags & CGIF_ATTR_HAS_TRANSPARENCY) || (pConfig->attrFlags & CGIF_FRAME_ATTR_HAS_ALPHA)) ? 1 : 0; // alpha channel is present
        hasSetTransp = (pConfig->attrFlags & CGIF_FRAME_ATTR_HAS_SET_TRANS) ? 1 : 0;  // user provided transparency setting (identical areas marked by user)
        // check for invalid configs:
        // cannot set alpha channel and user-provided transparency at the same time.
        if (hasAlpha && hasSetTransp) {
            pGIF->curResult = CGIF_ERROR;
            return pGIF->curResult;
        }
        // cannot set global and local alpha channel at the same time
        if ((pGIF->config.attrFlags & CGIF_ATTR_HAS_TRANSPARENCY) && (pConfig->attrFlags & CGIF_FRAME_ATTR_HAS_ALPHA)) {
            pGIF->curResult = CGIF_ERROR;
            return pGIF->curResult;
        }
        // sanity check:
        // at least one valid CT needed (global or local)
        if (!(pConfig->attrFlags & CGIF_FRAME_ATTR_USE_LOCAL_TABLE) && (pGIF->config.attrFlags & CGIF_ATTR_NO_GLOBAL_TABLE)) {
            pGIF->curResult = CGIF_ERROR;
            return CGIF_ERROR; // invalid config
        }

        // if frame matches previous frame, drop it completely and sum the frame delay
        if (pGIF->aFrames[pGIF->iHEAD] != NULL) {
            const uint32_t frameDelay = pConfig->delay + pGIF->aFrames[pGIF->iHEAD]->config.delay;
            if (frameDelay <= 0xFFFF && !(pGIF->config.genFlags & CGIF_GEN_KEEP_IDENT_FRAMES)) {
                int sameFrame = 1;
                for (i = 0; i < pGIF->config.width * pGIF->config.height; i++) {
                    if (cmpPixel(pGIF, pConfig, &pGIF->aFrames[pGIF->iHEAD]->config, pConfig->pImageData[i], pGIF->aFrames[pGIF->iHEAD]->config.pImageData[i])) {
                        sameFrame = 0;
                        break;
                    }
                }

                if (sameFrame) {
                    pGIF->aFrames[pGIF->iHEAD]->config.delay = frameDelay;
                    return CGIF_OK;
                }
            }
        }

        // search for free slot in frame queue
        for (i = pGIF->iHEAD; i < SIZE_FRAME_QUEUE && pGIF->aFrames[i] != NULL; ++i);
        // check whether the queue is full
        // when queue is full: we need to flush one frame.
        if (i == SIZE_FRAME_QUEUE) {
            r = flushFrame(pGIF, pGIF->aFrames[1], pGIF->aFrames[0]);
            freeFrame(pGIF->aFrames[0]);
            pGIF->aFrames[0] = NULL; // avoid potential double free in cgif_close
            // check for errors
            if (r != CGIF_OK) {
                pGIF->curResult = r;
                return pGIF->curResult;
            }
            i = SIZE_FRAME_QUEUE - 1;
            // keep the flushed frame in memory, as we might need it to write the next one.
            pGIF->aFrames[0] = pGIF->aFrames[1];
            pGIF->aFrames[1] = pGIF->aFrames[2];
        }
        // create new Frame struct + make a deep copy of pConfig.
        pNewFrame = (gif::CGIF_Frame*)malloc(sizeof(CGIF_Frame));
        copyFrameConfig(&(pNewFrame->config), pConfig);
        pNewFrame->config.pImageData = (uint8_t*)malloc(MULU16(pGIF->config.width, pGIF->config.height));
        memcpy(pNewFrame->config.pImageData, pConfig->pImageData, MULU16(pGIF->config.width, pGIF->config.height));
        // make a deep copy of the local color table, if required.
        if (pConfig->attrFlags & CGIF_FRAME_ATTR_USE_LOCAL_TABLE) {
            pNewFrame->config.pLocalPalette = (uint8_t*)malloc(pConfig->numLocalPaletteEntries * 3);
            memcpy(pNewFrame->config.pLocalPalette, pConfig->pLocalPalette, pConfig->numLocalPaletteEntries * 3);
        }
        pNewFrame->disposalMethod = DISPOSAL_METHOD_LEAVE;
        pNewFrame->transIndex = 0;
        pGIF->aFrames[i] = pNewFrame; // add frame to queue
        pGIF->iHEAD = i;         // update HEAD index
        // check whether we need to adapt the disposal method of the frame before.
        if (pGIF->config.attrFlags & CGIF_ATTR_HAS_TRANSPARENCY) {
            pGIF->aFrames[i]->disposalMethod = DISPOSAL_METHOD_BACKGROUND; // TBD might be removed
            pGIF->aFrames[i]->transIndex = 0;
            if (pGIF->aFrames[i - 1] != NULL) {
                pGIF->aFrames[i - 1]->config.genFlags &= ~(CGIF_FRAME_GEN_USE_TRANSPARENCY | CGIF_FRAME_GEN_USE_DIFF_WINDOW);
                pGIF->aFrames[i - 1]->disposalMethod = DISPOSAL_METHOD_BACKGROUND; // restore to background color
            }
        }
        // set per-frame alpha channel (we need to adapt the disposal method of the frame before)
        if (pConfig->attrFlags & CGIF_FRAME_ATTR_HAS_ALPHA) {
            pGIF->aFrames[i]->transIndex = pConfig->transIndex;
            if (pGIF->aFrames[i - 1] != NULL) {
                pGIF->aFrames[i - 1]->config.genFlags &= ~(CGIF_FRAME_GEN_USE_DIFF_WINDOW); // width/height optim not possible for frame before
                pGIF->aFrames[i - 1]->disposalMethod = DISPOSAL_METHOD_BACKGROUND; // restore to background color
            }
        }
        // user provided transparency setting
        if (hasSetTransp) {
            pGIF->aFrames[i]->transIndex = pConfig->transIndex;
        }
        pGIF->curResult = CGIF_OK;
        return pGIF->curResult;
    }

    /* close the GIF-file and free allocated space */
    int cgif_close(CGIF* pGIF) {
        int         r;
        cgif_result result;

        // check for previous errors
        if (pGIF->curResult != CGIF_OK) {
            goto CGIF_CLOSE_Cleanup;
        }

        // flush all remaining frames in queue
        for (int i = 1; i < SIZE_FRAME_QUEUE; ++i) {
            if (pGIF->aFrames[i] != NULL) {
                r = flushFrame(pGIF, pGIF->aFrames[i], pGIF->aFrames[i - 1]);
                if (r != CGIF_OK) {
                    pGIF->curResult = (gif::cgif_result)r;
                    break;
                }
            }
        }

        // cleanup
    CGIF_CLOSE_Cleanup:
        r = cgif_raw_close(pGIF->pGIFRaw); // close raw GIF stream
        // check for errors
        if (r != CGIF_OK) {
            pGIF->curResult = (gif::cgif_result)r;
        }

        if (pGIF->pFile) {
            r = fclose(pGIF->pFile); // we are done at this point => close the file
            if (r) {
                pGIF->curResult = CGIF_ECLOSE; // error: fclose failed
            }
        }
        for (int i = 0; i < SIZE_FRAME_QUEUE; ++i) {
            freeFrame(pGIF->aFrames[i]);
        }

        result = pGIF->curResult;
        freeCGIF(pGIF);
        // catch internal value CGIF_PENDING
        if (result == CGIF_PENDING) {
            result = CGIF_ERROR;
        }
        return result; // return previous result
    }

#define SIZE_MAIN_HEADER  (13)
#define SIZE_APP_EXT      (19)
#define SIZE_FRAME_HEADER (10)
#define SIZE_GRAPHIC_EXT  ( 8)

#define HEADER_OFFSET_SIGNATURE    (0x00)
#define HEADER_OFFSET_VERSION      (0x03)
#define HEADER_OFFSET_WIDTH        (0x06)
#define HEADER_OFFSET_HEIGHT       (0x08)
#define HEADER_OFFSET_PACKED_FIELD (0x0A)
#define HEADER_OFFSET_BACKGROUND   (0x0B)
#define HEADER_OFFSET_MAP          (0x0C)

#define IMAGE_OFFSET_LEFT          (0x01)
#define IMAGE_OFFSET_TOP           (0x03)
#define IMAGE_OFFSET_WIDTH         (0x05)
#define IMAGE_OFFSET_HEIGHT        (0x07)
#define IMAGE_OFFSET_PACKED_FIELD  (0x09)

#define IMAGE_PACKED_FIELD(a)      (*((uint8_t*) (a + IMAGE_OFFSET_PACKED_FIELD)))

#define APPEXT_OFFSET_NAME            (0x03)
#define APPEXT_NETSCAPE_OFFSET_LOOPS  (APPEXT_OFFSET_NAME + 13)

#define GEXT_OFFSET_DELAY          (0x04)

#define MAX_CODE_LEN    12                    // maximum code length for lzw
#define MAX_DICT_LEN    (1uL << MAX_CODE_LEN) // maximum length of the dictionary
#define BLOCK_SIZE      0xFF                  // number of bytes in one block of the image data

#define MULU16(a, b) (((uint32_t)a) * ((uint32_t)b)) // helper macro to correctly multiply two U16's without default signed int promotion

        typedef struct {
        uint8_t* pRasterData;
        uint32_t sizeRasterData;
    } LZWResult;

    typedef struct {
        uint16_t* pTreeInit;  // LZW dictionary tree for the initial dictionary (0-255 max)
        uint16_t* pTreeList;  // LZW dictionary tree as list (max. number of children per node = 1)
        uint16_t* pTreeMap;   // LZW dictionary tree as map (backup to pTreeList in case more than 1 child is present)
        uint16_t* pLZWData;   // pointer to LZW data
        const uint8_t* pImageData; // pointer to image data
        uint32_t        numPixel;   // number of pixels per frame
        uint32_t        LZWPos;     // position of the current LZW code
        uint16_t        dictPos;    // currrent position in dictionary, we need to store 0-4096 -- so there are at least 13 bits needed here
        uint16_t        mapPos;     // current position in LZW tree mapping table
    } LZWGenState;

    /* converts host U16 to little-endian (LE) U16 */
    static uint16_t hU16toLE(const uint16_t n) {
        int      isBE;
        uint16_t newVal;
        uint16_t one;

        one = 1;
        isBE = *((uint8_t*)&one) ? 0 : 1;
        if (isBE) {
            newVal = (n >> 8) | (n << 8);
        }
        else {
            newVal = n; // already LE
        }
        return newVal;
    }

    uint8_t calcNextPower2Ex(uint16_t n) {
        uint8_t nextPow2;

        for (nextPow2 = 0; n > (1uL << nextPow2); ++nextPow2);
        return nextPow2;
    }

    /* compute which initial LZW-code length is needed */
    static uint8_t calcInitCodeLen(uint16_t numEntries) {
        uint8_t index;

        index = calcNextPower2Ex(numEntries);
        return (index < 3) ? 3 : index + 1;
    }

    /* reset the dictionary of known LZW codes -- will reset the current code length as well */
    static void resetDict(LZWGenState* pContext, const uint16_t initDictLen) {
        pContext->dictPos = initDictLen + 2;                             // reset current position in dictionary (number of colors + 2 for start and end code)
        pContext->mapPos = 1;
        pContext->pLZWData[pContext->LZWPos] = initDictLen;                                 // issue clear-code
        ++(pContext->LZWPos);                                                               // increment position in LZW data
        // reset LZW list
        memset(pContext->pTreeInit, 0, initDictLen * sizeof(uint16_t) * initDictLen);
        memset(pContext->pTreeList, 0, ((sizeof(uint16_t) * 2) + sizeof(uint16_t)) * MAX_DICT_LEN);
    }

    /* add new child node */
    static void add_child(LZWGenState* pContext, const uint16_t parentIndex, const uint16_t LZWIndex, const uint16_t initDictLen, const uint8_t nextColor) {
        uint16_t* pTreeList;
        uint16_t  mapPos;

        pTreeList = pContext->pTreeList;
        mapPos = pTreeList[parentIndex * (2 + 1)];
        if (!mapPos) { // if pTreeMap is not used yet for the parent node
            if (pTreeList[parentIndex * (2 + 1) + 2]) { // if at least one child node exists, switch to pTreeMap
                mapPos = pContext->mapPos;
                // add child to mapping table (pTreeMap)
                memset(pContext->pTreeMap + ((mapPos - 1) * initDictLen), 0, initDictLen * sizeof(uint16_t));
                pContext->pTreeMap[(mapPos - 1) * initDictLen + nextColor] = LZWIndex;
                pTreeList[parentIndex * (2 + 1)] = mapPos;
                ++(pContext->mapPos);
            }
            else { // use the free spot in pTreeList for the child node
                pTreeList[parentIndex * (2 + 1) + 1] = nextColor; // color that leads to child node
                pTreeList[parentIndex * (2 + 1) + 2] = LZWIndex; // position of child node
            }
        }
        else { // directly add child node to pTreeMap
            pContext->pTreeMap[(mapPos - 1) * initDictLen + nextColor] = LZWIndex;
        }
        ++(pContext->dictPos); // increase current position in the dictionary
    }

    /* find next LZW code representing the longest pixel sequence that is still in the dictionary*/
    static int lzw_crawl_tree(LZWGenState* pContext, uint32_t* pStrPos, uint16_t parentIndex, const uint16_t initDictLen) {
        uint16_t* pTreeInit;
        uint16_t* pTreeList;
        uint32_t  strPos;
        uint16_t  nextParent;
        uint16_t  mapPos;

        if (parentIndex >= initDictLen) {
            return CGIF_EINDEX; // error: index in image data out-of-bounds
        }
        pTreeInit = pContext->pTreeInit;
        pTreeList = pContext->pTreeList;
        strPos = *pStrPos;
        // get the next LZW code from pTreeInit:
        // the initial nodes (0-255 max) have more children on average.
        // use the mapping approach right from the start for these nodes.
        if (strPos < (pContext->numPixel - 1)) {
            if (pContext->pImageData[strPos + 1] >= initDictLen) {
                return CGIF_EINDEX; // error: index in image data out-of-bounds
            }
            nextParent = pTreeInit[parentIndex * initDictLen + pContext->pImageData[strPos + 1]];
            if (nextParent) {
                parentIndex = nextParent;
                ++strPos;
            }
            else {
                pContext->pLZWData[pContext->LZWPos] = parentIndex; // write last LZW code in LZW data
                ++(pContext->LZWPos);
                if (pContext->dictPos < MAX_DICT_LEN) {
                    pTreeInit[parentIndex * initDictLen + pContext->pImageData[strPos + 1]] = pContext->dictPos;
                    ++(pContext->dictPos);
                }
                else {
                    resetDict(pContext, initDictLen);
                }
                ++strPos;
                *pStrPos = strPos;
                return CGIF_OK;
            }
        }
        // inner loop for codes > initDictLen
        while (strPos < (pContext->numPixel - 1)) {
            if (pContext->pImageData[strPos + 1] >= initDictLen) {
                return CGIF_EINDEX;  // error: index in image data out-of-bounds
            }
            // first try to find child in LZW list
            if (pTreeList[parentIndex * (2 + 1) + 2] && pTreeList[parentIndex * (2 + 1) + 1] == pContext->pImageData[strPos + 1]) {
                parentIndex = pTreeList[parentIndex * (2 + 1) + 2];
                ++strPos;
                continue;
            }
            // not found child yet? try to look into the LZW mapping table
            mapPos = pContext->pTreeList[parentIndex * (2 + 1)];
            if (mapPos) {
                nextParent = pContext->pTreeMap[(mapPos - 1) * initDictLen + pContext->pImageData[strPos + 1]];
                if (nextParent) {
                    parentIndex = nextParent;
                    ++strPos;
                    continue;
                }
            }
            // still not found child? add current parentIndex to LZW data and add new child
            pContext->pLZWData[pContext->LZWPos] = parentIndex; // write last LZW code in LZW data
            ++(pContext->LZWPos);
            if (pContext->dictPos < MAX_DICT_LEN) { // if LZW-dictionary is not full yet
                add_child(pContext, parentIndex, pContext->dictPos, initDictLen, pContext->pImageData[strPos + 1]); // add new LZW code to dictionary
            }
            else {
                // the dictionary reached its maximum code => reset it (not required by GIF-standard but mostly done like this)
                resetDict(pContext, initDictLen);
            }
            ++strPos;
            *pStrPos = strPos;
            return CGIF_OK;
        }
        pContext->pLZWData[pContext->LZWPos] = parentIndex; // if the end of the image is reached, write last LZW code
        ++(pContext->LZWPos);
        ++strPos;
        *pStrPos = strPos;
        return CGIF_OK;
    }

    /* generate LZW-codes that compress the image data*/
    static int lzw_generate(LZWGenState* pContext, uint16_t initDictLen) {
        uint32_t strPos;
        int      r;
        uint8_t  parentIndex;

        strPos = 0;                                                                          // start at beginning of the image data
        resetDict(pContext, initDictLen);                                            // reset dictionary and issue clear-code at first
        while (strPos < pContext->numPixel) {                                                 // while there are still image data to be encoded
            parentIndex = pContext->pImageData[strPos];                                       // start at root node
            // get longest sequence that is still in dictionary, return new position in image data
            r = lzw_crawl_tree(pContext, &strPos, (uint16_t)parentIndex, initDictLen);
            if (r != CGIF_OK) {
                return r; // error: return error code to callee
            }
        }
        pContext->pLZWData[pContext->LZWPos] = initDictLen + 1; // termination code
        ++(pContext->LZWPos);
        return CGIF_OK;
    }

    /* pack the LZW data into a byte sequence*/
    static uint32_t create_byte_list(uint8_t* byteList, uint32_t lzwPos, uint16_t* lzwStr, uint16_t initDictLen, uint8_t initCodeLen) {
        uint32_t i;
        uint32_t dictPos;                                                             // counting new LZW codes
        uint16_t n = 2 * initDictLen;                             // if n - initDictLen == dictPos, the LZW code size is incremented by 1 bit
        uint32_t bytePos = 0;                                                   // position of current byte
        uint8_t  bitOffset = 0;                                                   // number of bits used in the last byte
        uint8_t  lzwCodeLen = initCodeLen;                                 // dynamically increasing length of the LZW codes
        int      correctLater = 0;                                                   // 1: one empty byte too much if end is reached after current code, 0 otherwise

        byteList[0] = 0; // except from the 1st byte all other bytes should be initialized stepwise (below)
        // the very first symbol might be the clear-code. However, this is not mandatory. Quote:
        // "Encoders should output a Clear code as the first code of each image data stream."
        // We keep the option to NOT output the clear code as the first symbol in this function.
        dictPos = 1;
        for (i = 0; i < lzwPos; ++i) {                                                 // loop over all LZW codes
            if ((lzwCodeLen < MAX_CODE_LEN) && ((uint32_t)(n - (initDictLen)) == dictPos)) { // larger code is used for the 1st time at i = 256 ...+ 512 ...+ 1024 -> 256, 768, 1792
                ++lzwCodeLen;                                                             // increment the length of the LZW codes (bit units)
                n *= 2;                                                                   // set threshold for next increment of LZW code size
            }
            correctLater = 0;                                                     // 1 indicates that one empty byte is too much at the end
            byteList[bytePos] |= ((uint8_t)(lzwStr[i] << bitOffset));                   // add 1st bits of the new LZW code to the byte containing part of the previous code
            if (lzwCodeLen + bitOffset >= 8) {                                           // if the current byte is not enough of the LZW code
                if (lzwCodeLen + bitOffset == 8) {                                         // if just this byte is filled exactly
                    byteList[++bytePos] = 0;                                                // byte is full -- go to next byte and initialize as 0
                    correctLater = 1;                                                // use if one 0byte to much at the end
                }
                else if (lzwCodeLen + bitOffset < 16) {                                  // if the next byte is not completely filled
                    byteList[++bytePos] = (uint8_t)(lzwStr[i] >> (8 - bitOffset));
                }
                else if (lzwCodeLen + bitOffset == 16) {                                 // if the next byte is exactly filled by LZW code
                    byteList[++bytePos] = (uint8_t)(lzwStr[i] >> (8 - bitOffset));
                    byteList[++bytePos] = 0;                                                // byte is full -- go to next byte and initialize as 0
                    correctLater = 1;                                                // use if one 0byte to much at the end
                }
                else {                                                                  // lzw-code ranges over 3 bytes in total
                    byteList[++bytePos] = (uint8_t)(lzwStr[i] >> (8 - bitOffset));            // write part of LZW code to next byte
                    byteList[++bytePos] = (uint8_t)(lzwStr[i] >> (16 - bitOffset));           // write part of LZW code to byte after next byte
                }
            }
            bitOffset = (lzwCodeLen + bitOffset) % 8;                                   // how many bits of the last byte are used?
            ++dictPos;                                                                  // increment count of LZW codes
            if (lzwStr[i] == initDictLen) {                                      // if a clear code appears in the LZW data
                lzwCodeLen = initCodeLen;                                         // reset length of LZW codes
                n = 2 * initDictLen;                                     // reset threshold for next increment of LZW code length
                dictPos = 1;                                                              // reset (see comment below)
                // take first code already into account to increment lzwCodeLen exactly when the code length cannot represent the current maximum symbol.
                // Note: This is usually done implicitly, as the very first symbol is a clear-code itself.
            }
        }
        // comment: the last byte can be zero in the following case only:
        // terminate code has been written (initial dict length + 1), but current code size is larger so padding zero bits were added and extend into the next byte(s).
        if (correctLater) {                                                            // if an unneccessaray empty 0-byte was initialized at the end
            --bytePos;                                                                  // don't consider the last empty byte
        }
        return bytePos;
    }

    /* put byte sequence in blocks as required by GIF-format */
    static uint32_t create_byte_list_block(uint8_t* byteList, uint8_t* byteListBlock, const uint32_t numBytes) {
        uint32_t i;
        uint32_t numBlock = numBytes / BLOCK_SIZE;                                                    // number of byte blocks with length BLOCK_SIZE
        uint8_t  numRest = numBytes % BLOCK_SIZE;                                                    // number of bytes in last block (if not completely full)

        for (i = 0; i < numBlock; ++i) {                                                               // loop over all blocks
            byteListBlock[i * (BLOCK_SIZE + 1)] = BLOCK_SIZE;                                             // number of bytes in the following block
            memcpy(byteListBlock + 1 + i * (BLOCK_SIZE + 1), byteList + i * BLOCK_SIZE, BLOCK_SIZE);            // copy block from byteList to byteListBlock
        }
        if (numRest > 0) {
            byteListBlock[numBlock * (BLOCK_SIZE + 1)] = numRest;                                           // number of bytes in the following block
            memcpy(byteListBlock + 1 + numBlock * (BLOCK_SIZE + 1), byteList + numBlock * BLOCK_SIZE, numRest); // copy block from byteList to byteListBlock
            byteListBlock[1 + numBlock * (BLOCK_SIZE + 1) + numRest] = 0;                               // set 0 at end of frame
            return 1 + numBlock * (BLOCK_SIZE + 1) + numRest;                                           // index of last entry in byteListBlock
        }
        // all LZW blocks in the frame have the same block size (BLOCK_SIZE), so there are no remaining bytes to be writen.
        byteListBlock[numBlock * (BLOCK_SIZE + 1)] = 0;                                                // set 0 at end of frame
        return numBlock * (BLOCK_SIZE + 1);                                                            // index of last entry in byteListBlock
    }

    /* create all LZW raster data in GIF-format */
    static int LZW_GenerateStream(LZWResult* pResult, const uint32_t numPixel, const uint8_t* pImageData, const uint16_t initDictLen, const uint8_t initCodeLen) {
        LZWGenState* pContext;
        uint32_t     lzwPos, bytePos;
        uint32_t     bytePosBlock;
        int          r;
        // TBD recycle LZW tree list and map (if possible) to decrease the number of allocs
        pContext = (LZWGenState*)malloc(sizeof(LZWGenState)); // TBD check return value of malloc
        pContext->pTreeInit = (uint16_t*)malloc((initDictLen * sizeof(uint16_t)) * initDictLen); // TBD check return value of malloc
        pContext->pTreeList = (uint16_t*)malloc(((sizeof(uint16_t) * 2) + sizeof(uint16_t)) * MAX_DICT_LEN); // TBD check return value of malloc TBD check size
        pContext->pTreeMap = (uint16_t*)malloc(((MAX_DICT_LEN / 2) + 1) * (initDictLen * sizeof(uint16_t))); // TBD check return value of malloc
        pContext->numPixel = numPixel;
        pContext->pImageData = pImageData;
        pContext->pLZWData = (uint16_t*)malloc(sizeof(uint16_t) * (numPixel + 2)); // TBD check return value of malloc
        pContext->LZWPos = 0;

        // actually generate the LZW sequence.
        r = lzw_generate(pContext, initDictLen);
        if (r != CGIF_OK) {
            free(pContext->pLZWData);
            free(pContext->pTreeInit);
            free(pContext->pTreeList);
            free(pContext->pTreeMap);
            free(pContext);
            return r;
        }
        lzwPos = pContext->LZWPos;

        // pack the generated LZW data into blocks of 255 bytes
        uint8_t* byteList; // lzw-data packed in byte-list
        uint8_t* byteListBlock; // lzw-data packed in byte-list with 255-block structure
        uint64_t MaxByteListLen = MAX_CODE_LEN * lzwPos / 8ul + 2ul + 1ul; // conservative upper bound
        uint64_t MaxByteListBlockLen = MAX_CODE_LEN * lzwPos * (BLOCK_SIZE + 1ul) / 8ul / BLOCK_SIZE + 2ul + 1ul + 1ul; // conservative upper bound
        byteList = (uint8_t*)malloc(MaxByteListLen); // TBD check return value of malloc
        byteListBlock = (uint8_t*)malloc(MaxByteListBlockLen); // TBD check return value of malloc
        bytePos = create_byte_list(byteList, lzwPos, pContext->pLZWData, initDictLen, initCodeLen);
        bytePosBlock = create_byte_list_block(byteList, byteListBlock, bytePos + 1);
        free(byteList);
        pResult->sizeRasterData = bytePosBlock + 1; // save
        pResult->pRasterData = byteListBlock;

        free(pContext->pLZWData);
        free(pContext->pTreeInit);
        free(pContext->pTreeList);
        free(pContext->pTreeMap);
        free(pContext);
        return r;
    }

    /* initialize the header of the GIF */
    static void initMainHeader(const CGIFRaw_Config* pConfig, uint8_t* pHeader) {
        uint16_t width, height;
        uint8_t  pow2GlobalPalette;

        width = pConfig->width;
        height = pConfig->height;

        // set header to a clean state
        memset(pHeader, 0, SIZE_MAIN_HEADER);

        // set Signature field to value "GIF"
        pHeader[HEADER_OFFSET_SIGNATURE] = 'G';
        pHeader[HEADER_OFFSET_SIGNATURE + 1] = 'I';
        pHeader[HEADER_OFFSET_SIGNATURE + 2] = 'F';

        // set Version field to value "89a"
        pHeader[HEADER_OFFSET_VERSION] = '8';
        pHeader[HEADER_OFFSET_VERSION + 1] = '9';
        pHeader[HEADER_OFFSET_VERSION + 2] = 'a';

        // set width of screen (LE ordering)
        const uint16_t widthLE = hU16toLE(width);
        memcpy(pHeader + HEADER_OFFSET_WIDTH, &widthLE, sizeof(uint16_t));

        // set height of screen (LE ordering)
        const uint16_t heightLE = hU16toLE(height);
        memcpy(pHeader + HEADER_OFFSET_HEIGHT, &heightLE, sizeof(uint16_t));

        // init packed field
        if (pConfig->sizeGCT) {
            pHeader[HEADER_OFFSET_PACKED_FIELD] = (1 << 7); // M = 1 (see GIF specc): global color table is present
            // calculate needed size of global color table (GCT).
            // MUST be a power of two.
            pow2GlobalPalette = calcNextPower2Ex(pConfig->sizeGCT);
            pow2GlobalPalette = (pow2GlobalPalette < 1) ? 1 : pow2GlobalPalette;      // minimum size is 2^1
            pHeader[HEADER_OFFSET_PACKED_FIELD] |= ((pow2GlobalPalette - 1) << 0);    // set size of GCT (0 - 7 in header + 1)
        }
    }

    /* initialize NETSCAPE app extension block (needed for animation) */
    static void initAppExtBlock(uint8_t* pAppExt, uint16_t numLoops) {
        memset(pAppExt, 0, SIZE_APP_EXT);
        // set data
        pAppExt[0] = 0x21;
        pAppExt[1] = 0xFF; // start of block
        pAppExt[2] = 0x0B; // eleven bytes to follow

        // write identifier for Netscape animation extension
        pAppExt[APPEXT_OFFSET_NAME] = 'N';
        pAppExt[APPEXT_OFFSET_NAME + 1] = 'E';
        pAppExt[APPEXT_OFFSET_NAME + 2] = 'T';
        pAppExt[APPEXT_OFFSET_NAME + 3] = 'S';
        pAppExt[APPEXT_OFFSET_NAME + 4] = 'C';
        pAppExt[APPEXT_OFFSET_NAME + 5] = 'A';
        pAppExt[APPEXT_OFFSET_NAME + 6] = 'P';
        pAppExt[APPEXT_OFFSET_NAME + 7] = 'E';
        pAppExt[APPEXT_OFFSET_NAME + 8] = '2';
        pAppExt[APPEXT_OFFSET_NAME + 9] = '.';
        pAppExt[APPEXT_OFFSET_NAME + 10] = '0';
        pAppExt[APPEXT_OFFSET_NAME + 11] = 0x03; // 3 bytes to follow
        pAppExt[APPEXT_OFFSET_NAME + 12] = 0x01; // TBD clarify
        // set number of repetitions (animation; LE ordering)
        const uint16_t netscapeLE = hU16toLE(numLoops);
        memcpy(pAppExt + APPEXT_NETSCAPE_OFFSET_LOOPS, &netscapeLE, sizeof(uint16_t));
    }

    /* write numBytes dummy bytes */
    static int writeDummyBytes(cgif_write_fn* pWriteFn, void* pContext, int numBytes) {
        int rWrite = 0;
        const uint8_t dummyByte = 0;

        for (int i = 0; i < numBytes; ++i) {
            rWrite |= pWriteFn(pContext, &dummyByte, 1);
        }
        return rWrite;
    }

    CGIFRaw* cgif_raw_newgif(const CGIFRaw_Config* pConfig) {
        uint8_t  aAppExt[SIZE_APP_EXT];
        uint8_t  aHeader[SIZE_MAIN_HEADER];
        CGIFRaw* pGIF;
        int      rWrite;
        // check for invalid GCT size
        if (pConfig->sizeGCT > 256) {
            return NULL; // invalid GCT size
        }
        pGIF = (gif::CGIFRaw*)malloc(sizeof(CGIFRaw));
        if (!pGIF) {
            return NULL;
        }
        memcpy(&(pGIF->config), pConfig, sizeof(CGIFRaw_Config));
        // initiate all sections we can at this stage:
        // - main GIF header
        // - global color table (GCT), if required
        // - netscape application extension (for animation), if required
        initMainHeader(pConfig, aHeader);
        rWrite = pConfig->pWriteFn(pConfig->pContext, aHeader, SIZE_MAIN_HEADER);

        // GCT required? => write it.
        if (pConfig->sizeGCT) {
            rWrite |= pConfig->pWriteFn(pConfig->pContext, pConfig->pGCT, pConfig->sizeGCT * 3);
            uint8_t pow2GCT = calcNextPower2Ex(pConfig->sizeGCT);
            pow2GCT = (pow2GCT < 1) ? 1 : pow2GCT; // minimum size is 2^1
            const uint16_t numBytesLeft = ((1 << pow2GCT) - pConfig->sizeGCT) * 3;
            rWrite |= writeDummyBytes(pConfig->pWriteFn, pConfig->pContext, numBytesLeft);
        }
        // GIF should be animated? => init & write app extension header ("NETSCAPE2.0")
        // No loop? Don't write NETSCAPE extension.
        if ((pConfig->attrFlags & CGIF_RAW_ATTR_IS_ANIMATED) && !(pConfig->attrFlags & CGIF_RAW_ATTR_NO_LOOP)) {
            initAppExtBlock(aAppExt, pConfig->numLoops);
            rWrite |= pConfig->pWriteFn(pConfig->pContext, aAppExt, SIZE_APP_EXT);
        }
        // check for write errors
        if (rWrite) {
            free(pGIF);
            return NULL;
        }

        // assume error per default.
        // set to CGIF_OK by the first successful cgif_raw_addframe() call, as a GIF without frames is invalid.
        pGIF->curResult = CGIF_PENDING;
        return pGIF;
    }

    /* add new frame to the raw GIF stream */
    cgif_result cgif_raw_addframe(CGIFRaw* pGIF, const CGIFRaw_FrameConfig* pConfig) {
        uint8_t    aFrameHeader[SIZE_FRAME_HEADER];
        uint8_t    aGraphicExt[SIZE_GRAPHIC_EXT];
        LZWResult  encResult;
        int        r, rWrite;
        const int  useLCT = pConfig->sizeLCT; // LCT stands for "local color table"
        const int  isInterlaced = (pConfig->attrFlags & CGIF_RAW_FRAME_ATTR_INTERLACED) ? 1 : 0;
        uint16_t   numEffColors; // number of effective colors
        uint16_t   initDictLen;
        uint8_t    pow2LCT, initCodeLen;

        if (pGIF->curResult != CGIF_OK && pGIF->curResult != CGIF_PENDING) {
            return pGIF->curResult; // return previous error
        }
        // check for invalid LCT size
        if (pConfig->sizeLCT > 256) {
            pGIF->curResult = CGIF_ERROR; // invalid LCT size
            return pGIF->curResult;
        }

        rWrite = 0;
        // set frame header to a clean state
        memset(aFrameHeader, 0, SIZE_FRAME_HEADER);
        // set needed fields in frame header
        aFrameHeader[0] = ','; // set frame seperator
        if (useLCT) {
            pow2LCT = calcNextPower2Ex(pConfig->sizeLCT);
            pow2LCT = (pow2LCT < 1) ? 1 : pow2LCT; // minimum size is 2^1
            IMAGE_PACKED_FIELD(aFrameHeader) = (1 << 7);
            // set size of local color table (0-7 in header + 1)
            IMAGE_PACKED_FIELD(aFrameHeader) |= ((pow2LCT - 1) << 0);
            numEffColors = pConfig->sizeLCT;
        }
        else {
            numEffColors = pGIF->config.sizeGCT; // global color table in use
        }
        // encode frame interlaced?
        IMAGE_PACKED_FIELD(aFrameHeader) |= (isInterlaced << 6);

        // transparency in use? we might need to increase numEffColors
        if ((pGIF->config.attrFlags & (CGIF_RAW_ATTR_IS_ANIMATED)) && (pConfig->attrFlags & (CGIF_RAW_FRAME_ATTR_HAS_TRANS)) && pConfig->transIndex >= numEffColors) {
            numEffColors = pConfig->transIndex + 1;
        }

        // calculate initial code length and initial dict length
        initCodeLen = calcInitCodeLen(numEffColors);
        initDictLen = 1uL << (initCodeLen - 1);
        const uint8_t initialCodeSize = initCodeLen - 1;

        const uint16_t frameWidthLE = hU16toLE(pConfig->width);
        const uint16_t frameHeightLE = hU16toLE(pConfig->height);
        const uint16_t frameTopLE = hU16toLE(pConfig->top);
        const uint16_t frameLeftLE = hU16toLE(pConfig->left);
        memcpy(aFrameHeader + IMAGE_OFFSET_WIDTH, &frameWidthLE, sizeof(uint16_t));
        memcpy(aFrameHeader + IMAGE_OFFSET_HEIGHT, &frameHeightLE, sizeof(uint16_t));
        memcpy(aFrameHeader + IMAGE_OFFSET_TOP, &frameTopLE, sizeof(uint16_t));
        memcpy(aFrameHeader + IMAGE_OFFSET_LEFT, &frameLeftLE, sizeof(uint16_t));
        // apply interlaced pattern
        // TBD creating a copy of pImageData is not ideal, but changes on the LZW encoding would
        // be necessary otherwise.
        if (isInterlaced) {
            uint8_t* pInterlaced = (uint8_t*)malloc(MULU16(pConfig->width, pConfig->height));
            if (pInterlaced == NULL) {
                pGIF->curResult = CGIF_EALLOC;
                return pGIF->curResult;
            }
            uint8_t* p = pInterlaced;
            // every 8th row (starting with row 0)
            for (uint32_t i = 0; i < pConfig->height; i += 8) {
                memcpy(p, pConfig->pImageData + i * pConfig->width, pConfig->width);
                p += pConfig->width;
            }
            // every 8th row (starting with row 4)
            for (uint32_t i = 4; i < pConfig->height; i += 8) {
                memcpy(p, pConfig->pImageData + i * pConfig->width, pConfig->width);
                p += pConfig->width;
            }
            // every 4th row (starting with row 2)
            for (uint32_t i = 2; i < pConfig->height; i += 4) {
                memcpy(p, pConfig->pImageData + i * pConfig->width, pConfig->width);
                p += pConfig->width;
            }
            // every 2th row (starting with row 1)
            for (uint32_t i = 1; i < pConfig->height; i += 2) {
                memcpy(p, pConfig->pImageData + i * pConfig->width, pConfig->width);
                p += pConfig->width;
            }
            r = LZW_GenerateStream(&encResult, MULU16(pConfig->width, pConfig->height), pInterlaced, initDictLen, initCodeLen);
            free(pInterlaced);
        }
        else {
            r = LZW_GenerateStream(&encResult, MULU16(pConfig->width, pConfig->height), pConfig->pImageData, initDictLen, initCodeLen);
        }

        // generate LZW raster data (actual image data)
        // check for errors
        if (r != CGIF_OK) {
            pGIF->curResult = (gif::cgif_result)r;
            return (gif::cgif_result)r;
        }

        // check whether the Graphic Control Extension is required or not:
        // It's required for animations and frames with transparency.
        int needsGraphicCtrlExt = (pGIF->config.attrFlags & CGIF_RAW_ATTR_IS_ANIMATED) | (pConfig->attrFlags & CGIF_RAW_FRAME_ATTR_HAS_TRANS);
        // do things for animation / transparency, if required.
        if (needsGraphicCtrlExt) {
            memset(aGraphicExt, 0, SIZE_GRAPHIC_EXT);
            aGraphicExt[0] = 0x21;
            aGraphicExt[1] = 0xF9;
            aGraphicExt[2] = 0x04;
            aGraphicExt[3] = pConfig->disposalMethod;
            // set flag indicating that transparency is used, if required.
            if (pConfig->attrFlags & CGIF_RAW_FRAME_ATTR_HAS_TRANS) {
                aGraphicExt[3] |= 0x01;
                aGraphicExt[6] = pConfig->transIndex;
            }
            // set delay (LE ordering)
            const uint16_t delayLE = hU16toLE(pConfig->delay);
            memcpy(aGraphicExt + GEXT_OFFSET_DELAY, &delayLE, sizeof(uint16_t));
            // write Graphic Control Extension
            rWrite |= pGIF->config.pWriteFn(pGIF->config.pContext, aGraphicExt, SIZE_GRAPHIC_EXT);
        }

        // write frame
        rWrite |= pGIF->config.pWriteFn(pGIF->config.pContext, aFrameHeader, SIZE_FRAME_HEADER);
        if (useLCT) {
            rWrite |= pGIF->config.pWriteFn(pGIF->config.pContext, pConfig->pLCT, pConfig->sizeLCT * 3);
            const uint16_t numBytesLeft = ((1 << pow2LCT) - pConfig->sizeLCT) * 3;
            rWrite |= writeDummyBytes(pGIF->config.pWriteFn, pGIF->config.pContext, numBytesLeft);
        }
        rWrite |= pGIF->config.pWriteFn(pGIF->config.pContext, &initialCodeSize, 1);
        rWrite |= pGIF->config.pWriteFn(pGIF->config.pContext, encResult.pRasterData, encResult.sizeRasterData);

        // check for write errors
        if (rWrite) {
            pGIF->curResult = CGIF_EWRITE;
        }
        else {
            pGIF->curResult = CGIF_OK;
        }
        // cleanup
        free(encResult.pRasterData);
        return pGIF->curResult;
    }

    cgif_result cgif_raw_close(CGIFRaw* pGIF) {
        int         rWrite;
        cgif_result result;

        rWrite = pGIF->config.pWriteFn(pGIF->config.pContext, (unsigned char*)";", 1); // write term symbol
        // check for write errors
        if (rWrite) {
            pGIF->curResult = CGIF_EWRITE;
        }
        result = pGIF->curResult;
        free(pGIF);
        return result;
    }
}