## R_GNOME
## -------
AC_DEFUN([R_GNOME],
[ GNOME_INIT_HOOK([], [cont])
  if test "${GNOMEUI_LIBS}"; then
    AM_PATH_LIBGLADE([use_gnome="yes"
                      GNOME_IF_FILES="gnome-interface.glade"],
                     [warn_libglade_version="GNOME support requires libglade ver
sion >= 0.3"
                      AC_MSG_WARN([${warn_libglade_version}])],
                     [gnome])
  fi
if test "${use_gnome}" != yes; then
  use_gnome="no"
  GNOME_IF_FILES=
fi
AC_SUBST(GNOME_IF_FILES)
])# R_GNOME
