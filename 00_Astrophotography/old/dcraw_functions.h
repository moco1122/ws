/*
 * dcraw_functions.h
 *
 *  Created on: 2015/10/21
 *      Author: kawai
 */

#ifndef DCRAW_FUNCTIONS_H_
#define DCRAW_FUNCTIONS_H_

struct NEF {
	int aaa;
} ;

#ifdef __cplusplus
extern "C"
{
#endif

struct NEF readNEF(const char *fileName);

#ifdef __cplusplus
}
#endif

#endif /* DCRAW_FUNCTIONS_H_ */
