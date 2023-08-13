/* operator.h: description of operator ID
 * 
 */
#ifndef _OPERATOR_H_
#define _OPERATOR_H_

#define BRAND_STR_SIZ	32

void find_operator(int mcc_mnc, char *brand);
int match_vzw_plmn(int plmn);
#endif /*_OPERATOR_H_*/
