/*
 * This file was generated automatically by ExtUtils::ParseXS version 2.08 from the
 * contents of Variable.xs. Do not edit this file, edit Variable.xs instead.
 *
 *	ANY CHANGES MADE HERE WILL BE LOST! 
 *
 */

#line 1 "Variable.xs"
#include "module.h"

#ifndef PERL_UNUSED_VAR
#  define PERL_UNUSED_VAR(var) if (0) var = var
#endif

#line 17 "Variable.c"
XS(XS_Ekg2_variable_find); /* prototype to pass -Wmissing-prototypes */
XS(XS_Ekg2_variable_find)
{
    dXSARGS;
    if (items != 1)
	Perl_croak(aTHX_ "Usage: Ekg2::variable_find(name)");
    PERL_UNUSED_VAR(cv); /* -W */
    {
	Ekg2__Variable	RETVAL;
	const char *	name = (const char *)SvPV_nolen(ST(0));

	RETVAL = variable_find(name);
	ST(0) = (void *) bless_variable( (variable_t *) RETVAL);

	sv_2mortal(ST(0));
    }
    XSRETURN(1);
}

XS(XS_Ekg2_variables); /* prototype to pass -Wmissing-prototypes */
XS(XS_Ekg2_variables)
{
    dXSARGS;
    if (items != 0)
	Perl_croak(aTHX_ "Usage: Ekg2::variables()");
    PERL_UNUSED_VAR(cv); /* -W */
    PERL_UNUSED_VAR(ax); /* -Wall */
    SP -= items;
    {
#line 10 "Variable.xs"
        list_t l;
#line 49 "Variable.c"
#line 12 "Variable.xs"
        for (l = variables; l; l = l->next) {
                XPUSHs(sv_2mortal(bless_variable( (variable_t *) l->data)));
        }
#line 54 "Variable.c"
	PUTBACK;
	return;
    }
}

XS(XS_Ekg2__Variable_help); /* prototype to pass -Wmissing-prototypes */
XS(XS_Ekg2__Variable_help)
{
    dXSARGS;
    if (items != 1)
	Perl_croak(aTHX_ "Usage: Ekg2::Variable::help(var)");
    PERL_UNUSED_VAR(cv); /* -W */
    {
	Ekg2__Variable	var = (variable_t *) Ekg2_ref_object(ST(0));
#line 22 "Variable.xs"
	variable_help(var->name);
#line 71 "Variable.c"
    }
    XSRETURN_EMPTY;
}

XS(XS_Ekg2__Variable_set); /* prototype to pass -Wmissing-prototypes */
XS(XS_Ekg2__Variable_set)
{
    dXSARGS;
    if (items != 2)
	Perl_croak(aTHX_ "Usage: Ekg2::Variable::set(var, value)");
    PERL_UNUSED_VAR(cv); /* -W */
    {
	int	RETVAL;
	dXSTARG;
	Ekg2__Variable	var = (variable_t *) Ekg2_ref_object(ST(0));
	const char *	value = (const char *)SvPV_nolen(ST(1));
#line 26 "Variable.xs"
	variable_set(var->name, value, 0);
#line 90 "Variable.c"
    }
    XSRETURN(1);
}

XS(XS_Ekg2__Variable_new); /* prototype to pass -Wmissing-prototypes */
XS(XS_Ekg2__Variable_new)
{
    dXSARGS;
    if (items != 0)
	Perl_croak(aTHX_ "Usage: Ekg2::Variable::new()");
    PERL_UNUSED_VAR(cv); /* -W */
    {
	int	RETVAL;
	dXSTARG;
#line 30 "Variable.xs"
	debug("[VARIABLE.XS] variable_new() TODO\n");
#line 107 "Variable.c"
    }
    XSRETURN(1);
}

#ifdef __cplusplus
extern "C"
#endif
XS(boot_Ekg2__Variable); /* prototype to pass -Wmissing-prototypes */
XS(boot_Ekg2__Variable)
{
    dXSARGS;
    char* file = __FILE__;

    PERL_UNUSED_VAR(cv); /* -W */
    PERL_UNUSED_VAR(items); /* -W */
    XS_VERSION_BOOTCHECK ;

        newXSproto("Ekg2::variable_find", XS_Ekg2_variable_find, file, "$");
        newXSproto("Ekg2::variables", XS_Ekg2_variables, file, "");
        newXSproto("Ekg2::Variable::help", XS_Ekg2__Variable_help, file, "$");
        newXSproto("Ekg2::Variable::set", XS_Ekg2__Variable_set, file, "$$");
        newXSproto("Ekg2::Variable::new", XS_Ekg2__Variable_new, file, "");
    XSRETURN_YES;
}

