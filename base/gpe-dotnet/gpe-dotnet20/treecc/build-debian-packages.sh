#!/bin/bash
#
# SYPNOSIS
#   ./build-debian-packages.sh
#
# DESCRIPTION
#   This script combines the information in the rpm .spec file
#   with the build-debian-packages.conf file to produce most
#   of the control files in the debian directory.  This is done
#   so package descrptions and file lists can be maintained in
#   one place - the .spec file.  The files created in the
#   debian directory are:
#
#     changelog (updated if out of date)
#     control
#     copyright
#     *.docs
#     *.info
#     *.install
#
#   The debian source and packages are then built.  The end
#   result appears in the parent directory.
#
#   Changes and building are done in a copy of this directory
#   created in debian/tmp.  Apart from debian/tmp, no files
#   un this directory are modified.
#
# WARNING
# -------
#   Currently, this file is identical in all pnet packages.  It
#   is configured via the build-debian-packages.conf file.  If
#   you make changes to this file, copy it to all pnet modules
#   in the CVS repository.
#
set -e

#
# Get the version of an installed debian package.
#
getVersion() {
  local pkgVersion=$(dpkg-query --show --showformat='${Version}' $1)
  [[ -n "${pkgVersion}" ]] || {
    echo 1>&2 "$0: The latest version of $1 must be compiled and installed"
    echo 1>&2 "before you can create this package."
    exit 1
  }
  echo ${pkgVersion}
}

version=$(pwd)
version=${version##*-}

pnetDpkgVersion() {
  local dpkgVersion=$(getVersion "$1")
  [[ "${dpkgVersion%-*}" = "${version%-*}" ]] || {
    echo 1>&2 "$0: Must be built with $1 version ${version%-*}."
    exit 1
  }
  echo $dpkgVersion
}

#
# mcsversion is used by the ml-pnet package.
#
mcsversion=$(/bin/ls 2>/dev/null debian/mcs-*.tar.gz | tail -1 | sed 's/.*mcs-\(.*\).tar.gz/\1/')
fixMcsVersion() {
  echo -n "$1" | sed "s/%{mcsversion}/$mcsversion/g"
}

#
# Read in the configuration.
#
. build-debian-packages.conf

#
# Get a package override.
#
getOverride() {
  local package="$(echo "$1" | sed 's/\*/\\*/')"
  local overrides="$2"
  local default="$3"
  local override=$(echo "${overrides}" | sed -n "s;^ *${package}: *;;p")
  if [[ -n "${override}" ]]
  then echo "${override}"
  elif [[ -n "${default}" ]]
  then echo "${default}"
  fi
}

#
# Extract the package name from a RPM .spec line.
#
package() {
  local packageName
  case "$1" in
    *'	'*|*' '*)	packageName="${PKG_NAME}-${1##* }" ;;
    *)			packageName="${PKG_NAME}" ;;
  esac
  packageName=$(getOverride "${packageName}" "${PKG_MUNG}" "${packageName}")
  [[ "${packageName}" = DELETE ]] ||
    echo "${packageName}"
}

#
# Expand RPM %{_XXX} directory macros.
#
expandRpmMacros() {
  case "$1" in
    %{_bindir}*)	echo "usr/bin${1#%{_bindir\}}" ;;
    %{_datadir}*)	echo "usr/share${1#%{_datadir\}}" ;;
    %{_includedir}*)	echo "usr/include${1#%{_includedir\}}" ;;
    %{_infodir}*)	echo "usr/share/info${1#%{_infodir\}}" ;;
    %{_libdir}*)	echo "usr/lib${1#%{_libdir\}}" ;;
    %{_mandir}*)	echo "usr/share/man${1#%{_mandir\}}" ;;
    %{*)
	echo 1>&2 "$0: Unrecognised macro in '$1' at ${PKG_TAR}.spec.in line ${linenr}."
	exit 1
	;;
    *)			echo "$1"
  esac
}

#
# Debian requies the directory we operate in be called
# $PKG_NAME-version.  Some pnet tarballs don't follow
# that convention.  So force it by copying ourselves
# into a directory of that name.
#
rm -rf debian/tmp
builddir="debian/tmp/${PKG_NAME}-${version}"
mkdir -p "${builddir}"
tar --create --exclude="./debian/tmp" --exclude="./debian/tmp/*" . |
  tar --extract --directory "${builddir}" 

#
# Create the .orig.tar.gz file.
#
cd "${builddir%/*}"
tar --create --gzip --file "${PKG_NAME}_${version}.orig.tar.gz" "${builddir##*/}"
cd "${builddir##*/}"

#
# Build the control, *.install and *.docs files.
#
control=debian/control
(
  echo Source: ${PKG_NAME}
  section="$(getOverride SOURCE "${PKG_SECTION}")"
  [[ -z "${section}" ]] || echo Section: ${section}
  echo Priority: optional
  echo Maintainer: "Russell Stuart <russell-debian@stuart.id.au>"
  echo Build-Depends: ${PKG_BUILDDEPENDS}
  echo Standards-Version: 3.6.1
) >"${control}"

filepackage=
linenr=0
specname=$(echo *.spec.in)
while read line
do
  linenr=$((linenr + 1))
  firstword="${line%%[ 	]*}"
  case "${firstword}" in
    Summary:)
      summary="${line#*[ 	]}"
      ;;
    %description)
      descpackage=$(package "${line}")
      description=
      ;;
    %files)
      filepackage=$(package "${line}")
      [[ -z "$filepackage" ]] || 
	for suffix in docs info install
	do >"debian/${filepackage}.${suffix}"
	done
      ;;
    %changelog)
     break
     ;;
    %doc)
      [[ -n "${filepackage}" ]] || continue
      for file in ${line#${firstword}}
      do
	case "${file}" in
	  COPYING)	;;
	  INSTALL)	;;
	  *)		echo ${file} ;;
	esac
      done >> "debian/${filepackage}.docs"
      ;;
    %defattr*)
      ;;
    %dir)
      ;;
    %{_infodir}/*)
      [[ -n "${filepackage}" ]] || continue
      renamed=$(getOverride "${line}" "${PKG_RENAMES}" "${line}")
      echo >>"debian/${filepackage}.info" debian/tmp/$(expandRpmMacros "${renamed}")
      ;;
    %{*)
      [[ -n "${filepackage}" ]] || continue
      renamed=$(getOverride "${line}" "${PKG_RENAMES}" "${line}")
      echo >>"debian/${filepackage}.install" $(expandRpmMacros "${renamed}")
      ;;
    %*)
      [[ -n "${filepackage}" ]] || continue
      echo 1>&2 "$0: Unrecognised RPM command in '${firstword}' at ${PKG_TAR}.spec.in line ${linenr}."
      exit 1
      ;;
    '')
      [[ -z "${description}" ]] || {
	( echo
	  echo Package: "${descpackage}"
	  section="$(getOverride "${descpackage}" "${PKG_SECTION}")"
	  [[ -z "${section}" ]] || echo Section: ${section} >>"${control}"
	  depends="$(getOverride "${descpackage}" "${PKG_DEPENDS}" '${shlibs:Depends}')"
	  [[ ."${depends}" = ."NONE" ]] || echo Depends: ${depends}
	  recommends="$(getOverride "${descpackage}" "${PKG_RECOMMENDS}")"
	  [[ -z "${recommends}" ]] || echo Recommends: ${recommends}
	  suggests="$(getOverride "${descpackage}" "${PKG_SUGGESTS}")"
	  [[ -z "${suggests}" ]] || echo Suggests: ${suggests}
	  echo Architecture: $(getOverride "${descpackage}" "${PKG_ARCH}" 'any')
	  echo -n Description: ${summary}
	  echo -e "$(fixMcsVersion "${description}")"
	) >>"${control}"
	descpackage=
	description=
      }
      ;;
    *)
      if [[ -n "${descpackage}" ]]
      then
        description="${description}\n ${line}"
      elif [[ -n "${filepackage}" ]]
      then
	echo >>"debian/${filepackage}.install" "${line#/}"
      fi
      ;;
  esac
done <"${specname}"

#
# The copyright file has a date in it.  So we write that
# as well.
#
copyright=debian/copyright
cat >"${copyright}" <<..................................
This package was debianized by:
  Russell Stuart <russell-debian@stuart.id.au>
on:
  $(date '+%a, %e %b %Y %T %z.')

It was downloaded from:
  http://www.southern-storm.com.au/portable_net.html

Upstream Author:
  Southern Storm Software, Pty Ltd.

Copyright:
  This software is copyright (C) $(date +%Y)  Southern Storm Software, Pty Ltd.
  You are free to distribute this software under the terms of the GNU
  General Public License.

On Debian systems, the complete text of the GNU General Public
License can be found in /usr/share/common-licenses/GPL file.
..................................

#
# Add an entry into the changelog, if that hasn't been done already.
#
[[ -n "$(sed -n "s/^${PKG_NAME} (${version}-//p" debian/changelog)" ]] || {
  ( echo "${PKG_NAME} (${version}-1) unstable; urgency=low"
    echo
    echo "  * New upstream release"
    echo
    date "+ -- Russell Stuart <russell-debian@stuart.id.au>  %a, %e %b %Y %T %z"
    echo
  ) | cat - debian/changelog > debian/changelog.new
  mv debian/changelog.new debian/changelog
  changelogModified=yes
}

#
# Invoke the magic Debian build incantation.
#
fakeroot=
[[ $(id -u) = 0 ]] ||
  fakeroot="fakeroot -l /usr/lib/libfakeroot/libfakeroot.so.0"
${fakeroot} debian/rules clean
${fakeroot} dpkg-buildpackage

#
# Move the results up to the original parent directory.
#
cd ../../..
mv "debian/tmp/${PKG_NAME}_${version}"* ..

#
# Give a warning if we changed the Debian changelog.
#
cmp -s "${builddir}/debian/changelog" debian/changelog || {
  echo "$0: I modified debian/changelog.  Please commit the changed file to cvs."
  cp "${builddir}/debian/changelog" debian/changelog
}
