dnl ----------------------
dnl check Net-SNMP library
dnl ----------------------
AC_DEFUN([mstpd_CHECK_SNMP], [
	if test x"$with_snmp" != x"no"; then
		AC_PATH_TOOL([NETSNMP_CONFIG], [net-snmp-config], [no])
		if test x"$NETSNMP_CONFIG" = x"no"; then
			AC_MSG_ERROR([no Net-SNMP support found])
		fi

		NETSNMP_LIBS=`${NETSNMP_CONFIG} --agent-libs`
		NETSNMP_CFLAGS="`${NETSNMP_CONFIG} --base-cflags` -DNETSNMP_NO_INLINE"

		CFLAGS="$CFLAGS ${NETSNMP_CFLAGS}"
		LIBS="$LIBS ${NETSNMP_LIBS}"
		AC_MSG_CHECKING([whether compiler supports flag "${NETSNMP_CFLAGS} ${NETSNMP_LIBS}" from Net-SNMP])
		AC_LINK_IFELSE([AC_LANG_PROGRAM([
int main(void);
],
[
{
	return 0;
}
])],
			[AC_MSG_RESULT(yes)],
			[AC_MSG_RESULT(no)
				AC_MSG_ERROR([Net-SNMP supported, but unable to link])
		])
	fi
])
