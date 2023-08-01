/**
* Decoder adapted from: https://github.com/lecram/gifdec
* Encoder adapted from: https://github.com/dloebl/cgif
* NOTE: no support for GIF files that don't have a global color table, nor for the plain text extension (rarely used)
*/
#pragma once

#include <stdint.h>
#include <sys/types.h>

    namespace gif
    {

        /* calculate next power of two exponent of given number (n MUST be <= 256) */
        static uint8_t calcNextPower2Ex(uint16_t n);

        typedef struct gd_Palette {
            int size;
            uint8_t colors[0x100 * 3]; // all images are 3-channel by default
        } gd_Palette;

        typedef struct gd_GCE {
            uint16_t delay;
            uint8_t tindex;
            uint8_t disposal;
            int input;
            int transparency;
        } gd_GCE;

        typedef struct gd_GIF {
            int fd;
            off_t anim_start;
            uint16_t width, height;
            uint16_t depth;
            uint16_t loop_count;
            gd_GCE gce;
            gd_Palette* palette;
            gd_Palette lct, gct;
            void (*plain_text)(
                struct gd_GIF* gif, uint16_t tx, uint16_t ty,
                uint16_t tw, uint16_t th, uint8_t cw, uint8_t ch,
                uint8_t fg, uint8_t bg
                );
            void (*comment)(struct gd_GIF* gif);
            void (*application)(struct gd_GIF* gif, char id[8], char auth[3]);
            uint16_t fx, fy, fw, fh;
            uint8_t bgindex;
            uint8_t* canvas, * frame;
        } gd_GIF;

        gd_GIF* gd_open_gif(const char* fname);
        int gd_get_frame(gd_GIF* gif);
        void gd_render_frame(gd_GIF* gif, uint8_t* buffer);
        int gd_is_bgcolor(gd_GIF* gif, uint8_t color[3]);
        void gd_rewind(gd_GIF* gif);
        void gd_close_gif(gd_GIF* gif);

        // ================================================================================

        // flags to set the GIF / frame - attributes
#define CGIF_ATTR_IS_ANIMATED            (1uL << 1)       // make an animated GIF (default is non-animated GIF)
#define CGIF_ATTR_NO_GLOBAL_TABLE        (1uL << 2)       // disable global color table (global color table is default)
#define CGIF_ATTR_HAS_TRANSPARENCY       (1uL << 3)       // first entry in color table contains transparency (alpha channel)
#define CGIF_ATTR_NO_LOOP                (1uL << 4)       // don't loop a GIF animation: only play it one time.

#define CGIF_GEN_KEEP_IDENT_FRAMES       (1uL << 0)       // keep frames that are identical to previous frame (default is to drop them)

#define CGIF_FRAME_ATTR_USE_LOCAL_TABLE  (1uL << 0)       // use a local color table for a frame (local color table is not used by default)
#define CGIF_FRAME_ATTR_HAS_ALPHA        (1uL << 1)       // alpha channel index provided by user (transIndex field)
#define CGIF_FRAME_ATTR_HAS_SET_TRANS    (1uL << 2)       // transparency setting provided by user (transIndex field)
#define CGIF_FRAME_ATTR_INTERLACED       (1uL << 3)       // encode frame interlaced (default is not interlaced)
            // flags to decrease GIF-size
#define CGIF_FRAME_GEN_USE_TRANSPARENCY  (1uL << 0)       // use transparency optimization (setting pixels identical to previous frame transparent)
#define CGIF_FRAME_GEN_USE_DIFF_WINDOW   (1uL << 1)       // do encoding just for the sub-window that has changed from previous frame

#define CGIF_INFINITE_LOOP               (0x0000uL)       // for animated GIF: 0 specifies infinite loop

            typedef enum {
            CGIF_ERROR = -1, // something unspecified failed
            CGIF_OK = 0, // everything OK
            CGIF_EWRITE,     // writing GIF data failed
            CGIF_EALLOC,     // allocating memory failed
            CGIF_ECLOSE,     // final call to fclose failed
            CGIF_EOPEN,      // failed to open output file
            CGIF_EINDEX,     // invalid index in image data provided by user
            // internal section (values subject to change)
            CGIF_PENDING,
        } cgif_result;

        typedef struct st_gif                  CGIF;              // struct for the full GIF
        typedef struct st_gifconfig            CGIF_Config;       // global cofinguration parameters of the GIF
        typedef struct st_frameconfig          CGIF_FrameConfig;  // local configuration parameters for a frame

        typedef int cgif_write_fn(void* pContext, const uint8_t* pData, const size_t numBytes); // callback function for stream-based output

        // prototypes
        CGIF* cgif_newgif(CGIF_Config* pConfig);                  // creates a new GIF (returns pointer to new GIF or NULL on error)
        int   cgif_addframe(CGIF* pGIF, CGIF_FrameConfig* pConfig); // adds the next frame to an existing GIF (returns 0 on success)
        int   cgif_close(CGIF* pGIF);                          // close file and free allocated memory (returns 0 on success)

        // CGIF_Config type (parameters passed by user)
        // note: must stay AS IS for backward compatibility
        struct st_gifconfig {
            uint8_t* pGlobalPalette;                            // global color table of the GIF
            const char* path;                                      // path of the GIF to be created, mutually exclusive with pWriteFn
            uint32_t    attrFlags;                                 // fixed attributes of the GIF (e.g. whether it is animated or not)
            uint32_t    genFlags;                                  // flags that determine how the GIF is generated (e.g. optimization)
            uint16_t    width;                                     // width of each frame in the GIF
            uint16_t    height;                                    // height of each frame in the GIF
            uint16_t    numGlobalPaletteEntries;                   // size of the global color table
            uint16_t    numLoops;                                  // number of repetitons of an animated GIF (set to INFINITE_LOOP for infinite loop)
            cgif_write_fn* pWriteFn;                               // callback function for chunks of output data, mutually exclusive with path
            void* pContext;                                  // opaque pointer passed as the first parameter to pWriteFn
        };

        // CGIF_FrameConfig type (parameters passed by user)
        // note: must stay AS IS for backward compatibility
        struct st_frameconfig {
            uint8_t* pLocalPalette;                             // local color table of a frame
            uint8_t* pImageData;                                // image data to be encoded
            uint32_t  attrFlags;                                 // fixed attributes of the GIF frame
            uint32_t  genFlags;                                  // flags that determine how the GIF frame is created (e.g. optimization)
            uint16_t  delay;                                     // delay before the next frame is shown (units of 0.01 s)
            uint16_t  numLocalPaletteEntries;                    // size of the local color table
            uint8_t   transIndex;                                // introduced with V0.2.0
        };
  
#define DISPOSAL_METHOD_LEAVE      (1uL << 2)
#define DISPOSAL_METHOD_BACKGROUND (2uL << 2)
#define DISPOSAL_METHOD_PREVIOUS   (3uL << 2)

        // flags to set the GIF attributes
#define CGIF_RAW_ATTR_IS_ANIMATED     (1uL << 0) // make an animated GIF (default is non-animated GIF)
#define CGIF_RAW_ATTR_NO_LOOP         (1uL << 1) // don't loop a GIF animation: only play it one time.

// flags to set the Frame attributes
#define CGIF_RAW_FRAME_ATTR_HAS_TRANS  (1uL << 0) // provided transIndex should be set
#define CGIF_RAW_FRAME_ATTR_INTERLACED (1uL << 1) // encode frame interlaced

// CGIFRaw_Config type
// note: internal sections, subject to change.
        typedef struct {
            cgif_write_fn* pWriteFn;     // callback function for chunks of output data
            void* pContext;     // opaque pointer passed as the first parameter to pWriteFn
            uint8_t* pGCT;         // global color table of the GIF
            uint32_t       attrFlags;    // fixed attributes of the GIF (e.g. whether it is animated or not)
            uint16_t       width;        // effective width of each frame in the GIF
            uint16_t       height;       // effective height of each frame in the GIF
            uint16_t       sizeGCT;      // size of the global color table (GCT)
            uint16_t       numLoops;     // number of repetitons of an animated GIF (set to INFINITE_LOOP resp. 0 for infinite loop, use CGIF_ATTR_NO_LOOP if you don't want any repetition)
        } CGIFRaw_Config;

        // CGIFRaw_FrameConfig type
        // note: internal sections, subject to chage.
        typedef struct {
            uint8_t* pLCT;              // local color table of the frame (LCT)
            uint8_t* pImageData;        // image data to be encoded (indices to CT)
            uint32_t  attrFlags;         // fixed attributes of the GIF frame
            uint16_t  width;             // width of frame
            uint16_t  height;            // height of frame
            uint16_t  top;               // top offset of frame
            uint16_t  left;              // left offset of frame
            uint16_t  delay;             // delay before the next frame is shown (units of 0.01 s [cs])
            uint16_t  sizeLCT;           // size of the local color table (LCT)
            uint8_t   disposalMethod;    // specifies how this frame should be disposed after being displayed.
            uint8_t   transIndex;        // transparency index
        } CGIFRaw_FrameConfig;

        // CGIFRaw type
        // note: internal sections, subject to change.
        typedef struct {
            CGIFRaw_Config config;    // configutation parameters of the GIF (see above)
            cgif_result    curResult; // current result status of GIFRaw stream
        } CGIFRaw;

        // prototypes
        CGIFRaw* cgif_raw_newgif(const CGIFRaw_Config* pConfig);
        cgif_result cgif_raw_addframe(CGIFRaw* pGIF, const CGIFRaw_FrameConfig* pConfig);
        cgif_result cgif_raw_close(CGIFRaw* pGIF);
}