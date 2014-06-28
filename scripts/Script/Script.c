/*
 * This file was generated automatically by ExtUtils::ParseXS version 2.2210 from the
 * contents of Script.xs. Do not edit this file, edit Script.xs instead.
 *
 *	ANY CHANGES MADE HERE WILL BE LOST! 
 *
 */

#line 1 "Script.xs"
#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include <stdio.h>
#include <ctype.h>
#include <openssl/ssl.h>
#include <openssl/ripemd.h>
#include <openssl/rand.h>
#include <CBHDKeys.h>
#include <CBChecksumBytes.h>
#include <CBAddress.h>
#include <CBWIF.h>
#include <CBByteArray.h>
#include <CBBase58.h>
#include <CBScript.h>

//bool CBInitScriptFromString(CBScript * self, char * string)
char* scriptToString(CBScript* script){
	char* answer = (char *)malloc(CBScriptStringMaxSize(script)*sizeof(char));
	CBScriptToString(script, answer);
	return answer;

}

CBScript* stringToScript(char* scriptstring){
	CBScript* self;
	if(CBInitScriptFromString(self,scriptstring)){
		return self;
	}
	else{
		return NULL;
	}
}


//////////////////////// perl export functions /////////////
/* Return 1 if this script is multisig, 0 for else*/
// this function does not work
char* whatTypeOfScript(char* scriptstring){
	CBScript * script = CBNewScriptFromString(scriptstring);
	if(script == NULL){
		return "NULL";
	}
	if(CBScriptIsMultisig(script)){
		return "multisig";
	}
	else if(CBScriptIsP2SH(script)){
		return "p2sh";
	}
	else if(CBScriptIsPubkey(script)){
		return "pubkey";
	}
	else if(CBScriptIsKeyHash(script)){
		return "keyhash";
	}
	else{
		return "FAILED";
	}

}



char* addressToScript(char* addressString){
    CBByteArray * addrStr = CBNewByteArrayFromString(addressString, true);
    CBAddress * addr = CBNewAddressFromString(addrStr, false);

    CBScript * script = CBNewScriptPubKeyHashOutput(CBByteArrayGetData(CBGetByteArray(addr)) + 1);

    char* answer = (char *)malloc(CBScriptStringMaxSize(script)*sizeof(char));
    CBScriptToString(script, answer);
    CBFreeScript(script);
    //printf("Script = %s\n", answer);

    return answer;
}

// CBScript * CBNewScriptPubKeyOutput(uint8_t * pubKey);
char* pubkeyToScript (char* pubKeystring){
	// convert to uint8_t *
	CBByteArray * masterString = CBNewByteArrayFromString(pubKeystring, true);
	CBScript * script = CBNewScriptPubKeyOutput(CBByteArrayGetData(masterString));
	CBReleaseObject(masterString);

	return scriptToString(script);
}

//http://stackoverflow.com/questions/1503763/how-can-i-pass-an-array-to-a-c-function-in-perl-xs#1505355
//CBNewScriptMultisigOutput(uint8_t ** pubKeys, uint8_t m, uint8_t n);
//char* multisigToScript (char** multisigConcatenated,)
char* multisigToScript(SV* pubKeyArray,int mKeysInt, int nKeysInt) {
	uint8_t mKeys, nKeys;
	mKeys = (uint8_t)mKeysInt;
	nKeys = (uint8_t)nKeysInt;

	int n;
	I32 length = 0;
    if ((! SvROK(pubKeyArray))
    || (SvTYPE(SvRV(pubKeyArray)) != SVt_PVAV)
    || ((length = av_len((AV *)SvRV(pubKeyArray))) < 0))
    {
        return 0;
    }
    /* Create the array which holds the return values. */
	uint8_t** multisig = (uint8_t**)malloc(nKeysInt * sizeof(uint8_t*));

	for (n=0; n<=length; n++) {
		STRLEN l;

		char * fn = SvPV (*av_fetch ((AV *) SvRV (pubKeyArray), n, 0), l);

		CBByteArray * masterString = CBNewByteArrayFromString(fn, true);

		// this line should just assign a uint8_t * pointer
		multisig[n] = CBByteArrayGetData(masterString);

		CBReleaseObject(masterString);

	}
	CBScript* finalscript =  CBNewScriptMultisigOutput(multisig,mKeys,nKeys);

	return scriptToString(finalscript);
}

#line 136 "Script.c"
#ifndef PERL_UNUSED_VAR
#  define PERL_UNUSED_VAR(var) if (0) var = var
#endif

#ifndef PERL_ARGS_ASSERT_CROAK_XS_USAGE
#define PERL_ARGS_ASSERT_CROAK_XS_USAGE assert(cv); assert(params)

/* prototype to pass -Wmissing-prototypes */
STATIC void
S_croak_xs_usage(pTHX_ const CV *const cv, const char *const params);

STATIC void
S_croak_xs_usage(pTHX_ const CV *const cv, const char *const params)
{
    const GV *const gv = CvGV(cv);

    PERL_ARGS_ASSERT_CROAK_XS_USAGE;

    if (gv) {
        const char *const gvname = GvNAME(gv);
        const HV *const stash = GvSTASH(gv);
        const char *const hvname = stash ? HvNAME(stash) : NULL;

        if (hvname)
            Perl_croak(aTHX_ "Usage: %s::%s(%s)", hvname, gvname, params);
        else
            Perl_croak(aTHX_ "Usage: %s(%s)", gvname, params);
    } else {
        /* Pants. I don't think that it should be possible to get here. */
        Perl_croak(aTHX_ "Usage: CODE(0x%"UVxf")(%s)", PTR2UV(cv), params);
    }
}
#undef  PERL_ARGS_ASSERT_CROAK_XS_USAGE

#ifdef PERL_IMPLICIT_CONTEXT
#define croak_xs_usage(a,b)	S_croak_xs_usage(aTHX_ a,b)
#else
#define croak_xs_usage		S_croak_xs_usage
#endif

#endif

/* NOTE: the prototype of newXSproto() is different in versions of perls,
 * so we define a portable version of newXSproto()
 */
#ifdef newXS_flags
#define newXSproto_portable(name, c_impl, file, proto) newXS_flags(name, c_impl, file, proto, 0)
#else
#define newXSproto_portable(name, c_impl, file, proto) (PL_Sv=(SV*)newXS(name, c_impl, file), sv_setpv(PL_Sv, proto), (CV*)PL_Sv)
#endif /* !defined(newXS_flags) */

#line 188 "Script.c"

XS(XS_CBitcoin__Script_whatTypeOfScript); /* prototype to pass -Wmissing-prototypes */
XS(XS_CBitcoin__Script_whatTypeOfScript)
{
#ifdef dVAR
    dVAR; dXSARGS;
#else
    dXSARGS;
#endif
    if (items != 1)
       croak_xs_usage(cv,  "scriptstring");
    {
	char *	scriptstring = (char *)SvPV_nolen(ST(0));
	char *	RETVAL;
	dXSTARG;

	RETVAL = whatTypeOfScript(scriptstring);
	sv_setpv(TARG, RETVAL); XSprePUSH; PUSHTARG;
    }
    XSRETURN(1);
}


XS(XS_CBitcoin__Script_addressToScript); /* prototype to pass -Wmissing-prototypes */
XS(XS_CBitcoin__Script_addressToScript)
{
#ifdef dVAR
    dVAR; dXSARGS;
#else
    dXSARGS;
#endif
    if (items != 1)
       croak_xs_usage(cv,  "addressString");
    {
	char *	addressString = (char *)SvPV_nolen(ST(0));
	char *	RETVAL;
	dXSTARG;

	RETVAL = addressToScript(addressString);
	sv_setpv(TARG, RETVAL); XSprePUSH; PUSHTARG;
    }
    XSRETURN(1);
}


XS(XS_CBitcoin__Script_pubkeyToScript); /* prototype to pass -Wmissing-prototypes */
XS(XS_CBitcoin__Script_pubkeyToScript)
{
#ifdef dVAR
    dVAR; dXSARGS;
#else
    dXSARGS;
#endif
    if (items != 1)
       croak_xs_usage(cv,  "pubKeystring");
    {
	char *	pubKeystring = (char *)SvPV_nolen(ST(0));
	char *	RETVAL;
	dXSTARG;

	RETVAL = pubkeyToScript(pubKeystring);
	sv_setpv(TARG, RETVAL); XSprePUSH; PUSHTARG;
    }
    XSRETURN(1);
}


XS(XS_CBitcoin__Script_multisigToScript); /* prototype to pass -Wmissing-prototypes */
XS(XS_CBitcoin__Script_multisigToScript)
{
#ifdef dVAR
    dVAR; dXSARGS;
#else
    dXSARGS;
#endif
    if (items != 3)
       croak_xs_usage(cv,  "pubKeyArray, mKeysInt, nKeysInt");
    {
	SV *	pubKeyArray = ST(0);
	int	mKeysInt = (int)SvIV(ST(1));
	int	nKeysInt = (int)SvIV(ST(2));
	char *	RETVAL;
	dXSTARG;

	RETVAL = multisigToScript(pubKeyArray, mKeysInt, nKeysInt);
	sv_setpv(TARG, RETVAL); XSprePUSH; PUSHTARG;
    }
    XSRETURN(1);
}

#ifdef __cplusplus
extern "C"
#endif
XS(boot_CBitcoin__Script); /* prototype to pass -Wmissing-prototypes */
XS(boot_CBitcoin__Script)
{
#ifdef dVAR
    dVAR; dXSARGS;
#else
    dXSARGS;
#endif
#if (PERL_REVISION == 5 && PERL_VERSION < 9)
    char* file = __FILE__;
#else
    const char* file = __FILE__;
#endif

    PERL_UNUSED_VAR(cv); /* -W */
    PERL_UNUSED_VAR(items); /* -W */
#ifdef XS_APIVERSION_BOOTCHECK
    XS_APIVERSION_BOOTCHECK;
#endif
    XS_VERSION_BOOTCHECK ;

        newXS("CBitcoin::Script::whatTypeOfScript", XS_CBitcoin__Script_whatTypeOfScript, file);
        newXS("CBitcoin::Script::addressToScript", XS_CBitcoin__Script_addressToScript, file);
        newXS("CBitcoin::Script::pubkeyToScript", XS_CBitcoin__Script_pubkeyToScript, file);
        newXS("CBitcoin::Script::multisigToScript", XS_CBitcoin__Script_multisigToScript, file);
#if (PERL_REVISION == 5 && PERL_VERSION >= 9)
  if (PL_unitcheckav)
       call_list(PL_scopestack_ix, PL_unitcheckav);
#endif
    XSRETURN_YES;
}

