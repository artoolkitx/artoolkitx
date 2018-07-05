//
//  arRefineCorners.h
//  AR
//
//  Created by John Bell on 6/07/18.
//

#ifndef arRefineCorners_h
#define arRefineCorners_h

#ifdef __cplusplus
extern "C" {
#endif

    void arRefineCorners(double (*vertex)[4][2], unsigned char *buff, int width, int height);

#ifdef __cplusplus
}
#endif
#endif /* arRefineCorners_h */
