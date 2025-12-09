#!/bin/bash

APP_NAME=lammps-gui
DESTDIR=${PWD}/LAMMPS_GUI
SYSROOT="$1"
VERSION="$2"
SRCDIR="$3"

echo "Delete old files, if they exist"
rm -rvf ${DESTDIR}/LAMMPS_GUI ${DESTDIR}/LAMMPS-Win10-amd64*.exe

echo "Create staging area for deployment and populate"
DESTDIR=${DESTDIR} cmake --install .  --prefix "/"

# no static libs needed
rm -rvf ${DESTDIR}/lib ${DESTDIR}/bin/liblammps.dll
# but the generic LAMMPS library dll
wget https://download.lammps.org/lammps-gui/liblammps.dll
mv -v liblammps.dll ${DESTDIR}/bin/

skipdlls="msvcrt ADVAPI32 CFGMGR32 GDI32 KERNEL32 MPR NETAPI32 PSAPI SHELL32 USER32 USERENV UxTheme VERSION WS2_32 WSOCK32 d3d11 dwmapi liblammps msvcrt_ole32 dxgi IMM32 ole32 OLEAUT32 WINMM WTSAPI32 COMCTL32 PSAPI bcrypt CRYPT32 IPHLPAPI Secur32 api-ms-win-core-path-l1-1-0 WLDAP32"
echo "Copying required DLL files"
for dll in $(objdump -p *.exe | sed -n -e '/DLL Name:/s/^.*DLL Name: *//p' | sort | uniq)
do \
    doskip=0
    for skip in ${skipdlls}
    do \
        test ${dll} = ${skip}.dll && doskip=1
        test ${dll} = ${skip}.DLL && doskip=1        
    done
    test ${doskip} -eq 1 && continue
    test -f ${DESTDIR}/bin/${dll} || cp -v ${SYSROOT}/bin/${dll} ${DESTDIR}/bin || exit 1
done

echo "Copy required Qt plugins"
mkdir -p ${DESTDIR}/qt5plugins
for plugin in imageformats platforms styles
do \
    cp -r ${SYSROOT}/lib/qt5/plugins/${plugin} ${DESTDIR}/qt5plugins/
done

echo "Check dependencies of DLL files"
for dll in $(objdump -p ${DESTDIR}/bin/*.dll ${DESTDIR}/qt5plugins/*/*.dll | sed -n -e '/DLL Name:/s/^.*DLL Name: *//p' | sort | uniq)
do \
    doskip=0
    for skip in ${skipdlls}
    do \
        test ${dll} = ${skip}.dll && doskip=1
        test ${dll} = ${skip}.DLL && doskip=1        
    done
    test ${doskip} -eq 1 && continue
    test -f ${DESTDIR}/bin/${dll} || cp -v ${SYSROOT}/bin/${dll} ${DESTDIR}/bin || exit 1
done

for dll in $(objdump -p ${DESTDIR}/bin/*.dll ${DESTDIR}/qt5plugins/*/*.dll | sed -n -e '/DLL Name:/s/^.*DLL Name: *//p' | sort | uniq)
do \
    doskip=0
    for skip in ${skipdlls}
    do \
        test ${dll} = ${skip}.dll && doskip=1
        test ${dll} = ${skip}.DLL && doskip=1        
    done
    test ${doskip} -eq 1 && continue
    test -f ${DESTDIR}/bin/${dll} || cp -v ${SYSROOT}/bin/${dll} ${DESTDIR}/bin || exit 1
done

cat > ${DESTDIR}/bin/qt.conf <<EOF
[Paths]
Plugins = ../qt5plugins
EOF

cp -v lammps-gui-v${VERSION}.pdf ${DESTDIR}/LAMMPS-GUI-Manual.pdf
cp -v ${SRCDIR}/LICENSE ${DESTDIR}/LICENSE.txt
unix2dos ${DESTDIR}/LICENSE.txt
cp -v ${SRCDIR}/packaging/lammps-gui.nsis ${SRCDIR}/packaging/FileAssociation.nsh ${DESTDIR}
cp -v ${SRCDIR}/resources/lammps-gui.ico ${SRCDIR}/resources/icons/lammps-gui-banner.bmp ${DESTDIR}
revflag=$(git rev-parse --abbrev-ref HEAD)
pushd ${DESTDIR}
makensis -DMINGW="${SYSROOT}/bin/" -DVERSION="${VERSION}" -DBIT=64 -DLMPREV="${revflag}" \
         lammps-gui.nsis
popd
