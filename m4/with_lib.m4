# serial 20110726004


AC_DEFUN([AX_WITH_EXT_LIB], [dnl
    AC_REQUIRE([AC_ARG_WITH])
    m4_ifval($1,[dnl
    AC_ARG_WITH([$1], [AS_HELP_STRING([--with-$1=PATH],[specify path to $1 library installation @<:@default=auto@:>@])],
                         [ax_with_$1=$withval], [ax_with_$1=auto])
    ])
])

AC_DEFUN([AX_SETUP_EXT_LIB],[dnl
    AS_REQUIRE([AS_IF])
    AS_REQUIRE([AS_CASE])

    m4_ifval($1, [dnl
        AS_VAR_PUSHDEF([ax_tmp_with],[ax_with_$1])
        AS_VAR_IF([ax_tmp_with],[auto],[:],[dnl
            AS_VAR_SET([ax_tmp_with], [`echo $ax_tmp_with | sed -e 's,/$,,'`])
            AS_CASE(["$ax_tmp_with"],[./*|../*],[ax_tmp_with="`pwd`/$ax_tmp_with"]) #''
            AS_IF([test -d "${ax_tmp_with}/lib"],[dnl
				AS_VAR_SET([m4_toupper([$1_LD_LIBRARY_PATH])],["${ax_tmp_with}/lib"])
                AS_VAR_SET(m4_toupper([$1_LDFLAGS]),["-L${ax_tmp_with}/lib"])
            ])
            AS_IF([test -d "${ax_tmp_with}/lib64"],[dnl
				AS_IF([test 0`echo $target | grep -c 64` -gt 0],[
					AS_VAR_SET([m4_toupper([$1_LD_LIBRARY_PATH])],["${ax_tmp_with}/lib64"])
                    AS_VAR_SET([m4_toupper([$1_LDFLAGS])],["-L${ax_tmp_with}/lib64"])
				],[dnl 
					AS_IF([test 0`echo $build | grep -c 64` -gt 0 ],[
						AS_VAR_SET([m4_toupper([$1_LD_LIBRARY_PATH])],["${ax_tmp_with}/lib64"])
						AS_VAR_SET([m4_toupper([$1_LDFLAGS])],["-L${ax_tmp_with}/lib64"])
					])
				])
            ])
            AS_IF([test -d "${ax_tmp_with}/include"],[dnl
                AS_VAR_SET([m4_toupper([$1_CPPFLAGS])],["-I${ax_tmp_with}/include"])
            ])
        ])
        AS_VAR_POPDEF([ax_tmp_with])
    ])
])

m4_define([ax_store_val],[m4_define([$1],[$2])])

AC_DEFUN([ax_make_tmpvars],[dnl
    AS_REQUIRE([_AS_TR_SH_PREPARE]) dnl
    ax_store_val([ax_extlib_varname],[m4_toupper([$1_$2])]) dnl
    m4_if([ax_extlib_varname],[_],[:],[ dnl
        AS_VAR_SET_IF([ax_extlib_varname],[ dnl
            ax_store_val([variable_payload],[m4_unquote([AX_SH_ACTION([ax_extlib_varname])])]) dnl
            AC_MSG_CHECKING([$1 flags ax_extlib_varname])
			AC_MSG_RESULT(m4_bpatsubst(m4_bpatsubst(variable_payload,[\"],[]),[\s],[]))
            m4_apply([AC_SUBST],[ax_extlib_varname])
        ])
    ])
])

AC_DEFUN([AX_SET_EXTLIB],[dnl
    AS_REQUIRE([_AS_TR_SH_PREPARE])
    m4_ifval($1,[$1],[dnl
          AC_DEFINE(m4_toupper([HAVE_LIB$2]), 1, [Define to 1 if you have lib$2])
          AS_VAR_SET([m4_toupper([$2_LIBS])], [-l$2])
          m4_map([ax_make_tmpvars], [[[$2],[LIBS]],[[$2],[LDFLAGS]],[[$2],[LD_LIBRARY_PATH]],[[$2],[CPPFLAGS]]])
    ])
])

dnl This only really ensures that the header includes the class definition; it will not ensure that the library contains it
dnl This has the advantage of being able to verify the existence of classes with private or deleted default constructors,
dnl but the disadvantage that the headers may mismatch with the library. 
AC_DEFUN([AX_CHECK_LIB_CLASS],
		 [AC_REQUIRE([AC_LINK_IFELSE])
          AC_REQUIRE([AC_LANG_PROGRAM])
          AC_LINK_IFELSE([AC_LANG_PROGRAM([#include $2],[typedef $3 foo; foo * bar; (void)(bar);])],[$4],[$5])
         ])

AC_DEFUN([AX_CHECK_EXT_LIB_CLASS],
         [AC_REQUIRE([AX_WITH_EXT_LIB])
          AC_REQUIRE([AX_SETUP_EXT_LIB])
          AC_REQUIRE([AX_CHECK_LIB_CLASS])
          AX_WITH_EXT_LIB([$1])
          AX_SETUP_EXT_LIB([$1])
          AS_VAR_SET([tmp_LDFLAGS],["$LDFLAGS"])
          AS_VAR_SET([LDFLAGS], ["$LDFLAGS m4_toupper([$$1_LDFLAGS])"])
          AS_VAR_SET([tmp_CPPFLAGS],["$CPPFLAGS"])
          AS_VAR_SET([CPPFLAGS], ["$CPPFLAGS m4_toupper([$$1_CPPFLAGS])"])
          AX_CHECK_LIB_CLASS([$1],[$2],[$3],[AX_SET_EXTLIB($4,$1)], [dnl
			   AS_VAR_SET([m4_toupper([$1_LIBS])], [-l$1]) 
			   m4_map([ax_make_tmpvars], [[[$1],[LIBS]]])
               $5
          ])
		  AS_VAR_SET([LDFLAGS],["$tmp_LDFLAGS"])
          AS_VAR_SET([CPPFLAGS], ["$tmp_CPPFLAGS"])
         ]
)

                 
AC_DEFUN([AX_CHECK_EXT_LIB],
         [AC_REQUIRE([AC_CHECK_LIB])
          AC_REQUIRE([AX_WITH_EXT_LIB])
          AC_REQUIRE([AX_SETUP_EXT_LIB])
          AX_WITH_EXT_LIB([$1])
          AX_SETUP_EXT_LIB([$1])
          AS_VAR_SET([tmp_LDFLAGS],["$LDFLAGS"])
          AS_VAR_SET([LDFLAGS], ["$LDFLAGS m4_toupper([$$1_LDFLAGS])"])
          AC_CHECK_LIB([$1],[$2],[AX_SET_EXTLIB($3,$1)], [dnl
			AS_VAR_SET([m4_toupper([$1_LIBS])], [-l$1])
			m4_map([ax_make_tmpvars], [[[$2],[LIBS]]])
			$4
		  ])
          AS_VAR_SET([LDFLAGS],["$tmp_LDFLAGS"])
         ])

