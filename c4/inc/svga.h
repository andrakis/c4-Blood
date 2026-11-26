// svga.h -- Smacker's SVGA helpers, which smkplay.cpp calls around a
// cutscene. With no Smacker decoder (smack.h) they are never reached for a
// frame; SVGASetup and SVGASetGraph return the game to its own mode.
#ifndef C4BLOOD_SVGA_H
#define C4BLOOD_SVGA_H 1
#ifdef __cplusplus
extern "C" {
#endif
extern unsigned int WidthToUse, HeightToUse;
unsigned int SVGADetect (unsigned int flags);
unsigned int SVGASetup (unsigned int width, unsigned int height);
void SVGASetGraph (void);
void SVGASetText (void);
void SVGASetPalette (void *pal);
unsigned int SVGABytesPS (void);
void *SVGAWinTbl (void);
void *SVGASetBank (void);
#ifdef __cplusplus
}
#endif
#endif
