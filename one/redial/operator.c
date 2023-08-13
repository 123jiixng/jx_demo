/* operator.c: find operator infomation */
#include "stdio.h"
#include "string.h"
#include "strings.h"
#include "operator.h"

typedef struct{
        int mcc_mnc;
        char brand[BRAND_STR_SIZ];
}OPERATOR_ID;

OPERATOR_ID operators[] = {
	/* Test*/
	{101, "Test"},
	/*China*/
	{46000, "China Mobile"},
	{46001, "China Unicom"},
	{46002, "China Mobile"},
	{46003, "China Telecom"},
	{46005, "China Telecom"},
	{46006, "China Unicom"},
	{46007, "China Mobile"},
	{46011, "China Telecom"},
	{46020, "China Tietong"},
	/*Spain - ES*/
	{21401, "Vodafone"},
	{21403, "Orange"},
	{21404, "Yoigo"},
	{21405, "TME"},
	{21406, "Vodafone"},
	{21407, "Movistar"},
	{21408, "Euskaltel"},
	{21409, "Orange"},
	{21415, "BT"},
	{21416, "TeleCable"},
	{21417, "Mobile R"},
	{21418, "ONO"},
	{21419, "Simyo"},
	{21420, "Fonyou"},
	{21421, "Jazztel"},
	{21422, "DigiMobil"},
	{21423, "Barablu"},
	{21424, "Eroski"},
	{21425, "LycaMobile"},
	/*Argentina - AR*/
	{722010, "Movistar"},
	{722020, "Nextel"},
	{722040, "Globalstar"},
	{722070, "Movistar"},
	{722310, "Claro"},
	{722320, "Claro"},
	{722330, "Claro"},
	{722350, "PORT-HABLE"},
	/* Australia - AU*/
	{50501, "Telstra"},
	{50502, "Optus"},
	{50503, "Vodafone"},
	{50514, "AAPT"},
	{50538, "Crazy John's"},
	{50571, "Telstra"},
	{50572, "Telstra"},
	{50590, "Optus"},
	/*Austria - AT*/
	{23201, "A1 TA"},
	{23202, "A1 TA"},
	{23203, "T-Mobile AT"},
	{23205, "Orange AT"},
	{23206, "Orange AT"},
	{23210, "3AT"},
	/*Belarus - BY*/
	{25701, "Velcom"},
	{25702, "MTS"},
	{25703, "DIALLOG"},
	{25704, "Life:)"},
	/*Belgium - BE*/
	{20601, "Proximus"},
	{20605, "Telenet"},
	{20610, "Mobistar"},
	{20620, "BASE"},
	/*Barzil - BR*/
	{72400, "Nextel"},
	{72439, "Nextel"},
	{72402, "TIM"},
	{72403, "TIM"},
	{72404, "TIM"},
	{72405, "Claro BR"},
	{72406, "Vivo"},
	{72410, "Vivo"},
	{72411, "Vivo"},
	{72423, "Vivo"},
	{72415, "CTBC Celular"},
	{72432, "CTBC Celular"},
	{72433, "CTBC Celular"},
	{72434, "CTBC Celular"},
	{72416, "Brasil Telecom"},
	/*Canada - CA*/
	{302220, "Telus"},
	{302221, "Telus"},
	{302361, "Telus"},
	{302653, "Telus"},
	{302657, "Telus"},
	{302610, "Bell"},
	{302640, "Bell"},
	{302370, "Fido"},
	{302610, "Bell"},
	{302610, "Bell"},
	/*France - FR*/
	{20801, "Orange"},
	{20802, "Orange"},
	{20810, "SFR"},
	{20811, "SFR"},
	{20813, "SFR"},
	{20820, "Bouygues"},
	{20821, "Bouygues"},
	{20888, "Bouygues"},
	{20823, "Virgin Mobile"},
	/*Germany - DE*/
	{26201, "T-Mobile"},
	{26206, "T-Mobile"},
	{26202, "Vodafone"},
	{26204, "Vodafone"},
	{26209, "Vodafone"},
	{26203, "E-Plus"},
	{26205, "E-Plus"},
	{26277, "E-Plus"},
	{26207, "O2"},
	{26208, "O2"},
	{26211, "O2"},
	/*Ghana - GH */
	{62001, "MTN"},
	{62002, "Vodafone"},
	/*Gibraltar - GI*/
	{26601, "GibTel"},
	{26606, "CTS Mobile"},
	/*Greece - GR*/
	{20201, "Cosmote"},
	{20205, "Vodafone"},
	{20209, "Wind"},
	{20210, "Wind"},
	/*HK*/
	{45400, "1O1O/One2Free"},
	{45403, "3"},
	{45406, "SmarTone-Vodafone"},
	{45407, "China Unicom"},
	{45412, "CMCC HK"},
	{45419, "PCCW Mobile"},
	{45429, "PCCW Mobile"},
	/*Hungary - HU*/
	{21601, "Telenor"},
	{21630, "T-Mobile"},
	{21670, "Vodafone"},
	/*Iceland - IS*/
	{27401, "Siminn"},
	{27402, "Vodafone"},
	{27403, "Vodafone"},
	/*Italy - IT*/
	{22201, "TIM"},
	{22210, "Vodafone"},
	/*Netherlands*/
	{20402, "Tele2"},
	{20416, "T-Mobile(BEN)"},
	{20420, "T-Mobile"},
	/*New Zealand - NZ*/
	{53001, "Vodafone"},
	{53002, "Telecom"},
	{53003, "Woosh"},
	/*Romania - RO*/
	{22601, "Vodafone"},
	{22610, "Orange"},
	/*Russian*/
	{25001, "MTS"},
	{25002, "MegaFon"},
	{25011, "Yota"},
	/*United Kingdom - UK*/
	{23400, "BT"},
	{23401, "Vection Mobile"},
	{23402, "O2(UK)"},
	{23410, "O2(UK)"},
	{23411, "O2(UK)"},
	{23412, "Railtrack"},
	{23413, "Railtrack"},
	{23415, "Vodafone UK"},
	{23430, "T-Mobile(UK)"},
	{23431, "Virgin Mobile UK"},
	{23432, "Virgin Mobile UK"},
	{23433, "Orange(UK)"},
	{23434, "Orange(UK)"},
	/*USA*/
	{310004, "Verizon"},
	{310005, "Verizon"},
	{310012, "Verizon"},
	{310026, "T-Mobile"},
	{310150, "AT&T"},
	{310260, "T-Mobile"},
	{310380, "AT&T"},
	{310410, "AT&T"},
	{310560, "AT&T"},
	{310680, "AT&T"},
	{311480, "Verizon"},
	{311481, "Verizon"},
	{311482, "Verizon"},
	{311483, "Verizon"},
	{311484, "Verizon"},
	{311485, "Verizon"},
	{311486, "Verizon"},
	{311487, "Verizon"},
	{311488, "Verizon"},
	{311489, "Verizon"},
	{316010, "Nextel"},
	{42701,  "Ooredoo"},
	/*Reserved*/
	{-1,""}
};

/* find operator by id
 * mcc_mnc: input,	MCC and MNC ID string got from the modem
 * brand: output,	Network Brand of the given ID, the length 
 *			of brand buffer should not be shorter than BRAND_STR_SIZ.
 */
void find_operator(int mcc_mnc, char *brand)
{
	OPERATOR_ID *ret = &operators[0];

	while(ret->mcc_mnc > 0){
		if (mcc_mnc == ret->mcc_mnc){
			strlcpy(brand, ret->brand, BRAND_STR_SIZ);
			return;
		}
		ret++;
	}
	sprintf(brand, "%d", mcc_mnc);
	return;
}

//Verizon wireless plmn list
const int gl_vzw_plmn_list[] = {
	310010,
	310012,
	310013,
	310350,
	310590,
	310591,
	310592,
	310593,
	310594,
	310595,
	310596,
	310597,
	310598,
	310599,
	310820,
	310890,
	310891,
	310892,
	310893,
	310894,
	310895,
	310896,
	310897,
	310898,
	310899,
	310910,
	311110,
	311270,
	311271,
	311272,
	311273,
	311274,
	311275,
	311276,
	311277,
	311278,
	311279,
	311280,
	311281,
	311282,
	311283,
	311284,
	311285,
	311286,
	311287,
	311288,
	311289,
	311390,
	311480,
	311481,
	311482,
	311483,
	311484,
	311485,
	311486,
	311487,
	311488,
	311489,
	311590,
	312770,
	-1
};

//function: match verizon wireless plmn
//return: 0 -> not matched, 1 -> matched
int match_vzw_plmn(int plmn)
{
	int *p = NULL;

	if(!plmn) return 0;
	
	for(p = (int *)gl_vzw_plmn_list; *p!=-1; p++){
		if(*p == plmn) return 1;
	}

	return 0;	
}

