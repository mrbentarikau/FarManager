#!/bin/bash

M4CMD="m4 -P -DFARBIT=$DIRBIT -DHOSTTYPE=Unix -DBUILD_TYPE=VS_RELEASE"

function buildfar2 {

OUTDIR=Release.$1.vc
export BOOTSTRAPDIR=$OUTDIR/obj/include/bootstrap/
DIRBIT=$1
BINDIR=outfinalnew${DIRBIT}

rm -fR $BINDIR

mkdir $BINDIR
mkdir -p $BINDIR/PluginSDK/Headers.c
mkdir -p $BINDIR/PluginSDK/Headers.pas

cd $2 || return 1

mkdir -p $OUTDIR
mkdir -p $OUTDIR/obj
mkdir -p $OUTDIR/cod
mkdir -p ${BOOTSTRAPDIR}

ls *.cpp *.hpp *.c *.rc | gawk -f ./scripts/mkdep.awk - | unix2dos > ${BOOTSTRAPDIR}far.vc.dep

$M4CMD farlang.templ.m4 > ${BOOTSTRAPDIR}farlang.templ
$M4CMD far.rc.inc.m4 > ${BOOTSTRAPDIR}far.rc.inc
$M4CMD Far.exe.manifest.m4 > ${BOOTSTRAPDIR}Far.exe.manifest
$M4CMD farversion.inc.m4 > ${BOOTSTRAPDIR}farversion.inc
pushd ../far.git/far
$M4CMD copyright.inc.m4 > ../../far/${BOOTSTRAPDIR}copyright.inc
popd
$M4CMD File_id.diz.m4 | unix2dos -m > $OUTDIR/File_id.diz

dos2unix FarEng.hlf.m4
dos2unix FarRus.hlf.m4
dos2unix FarHun.hlf.m4
dos2unix FarPol.hlf.m4
gawk -f ./scripts/mkhlf.awk FarEng.hlf.m4 | $M4CMD | unix2dos -m > $OUTDIR/FarEng.hlf
gawk -f ./scripts/mkhlf.awk FarRus.hlf.m4 | $M4CMD | unix2dos -m > $OUTDIR/FarRus.hlf
gawk -f ./scripts/mkhlf.awk FarHun.hlf.m4 | $M4CMD | unix2dos -m > $OUTDIR/FarHun.hlf
gawk -f ./scripts/mkhlf.awk FarPol.hlf.m4 | $M4CMD | unix2dos -m > $OUTDIR/FarPol.hlf

wine tools/lng.generator.exe -nc -oh ${BOOTSTRAPDIR} -ol $OUTDIR ${BOOTSTRAPDIR}farlang.templ

wine cmd /c ../mysetnew.${DIRBIT}.bat

cd ..

( \
cp $2/$OUTDIR/File_id.diz $2/$OUTDIR/Far.exe $2/$OUTDIR/*.hlf $2/$OUTDIR/Far.map $2/$OUTDIR/Far.pdb $2/$OUTDIR/*.lng $BINDIR/ && \
cp $2/Include/*.hpp $BINDIR/PluginSDK/Headers.c/ && \
cp $2/Include/*.pas $BINDIR/PluginSDK/Headers.pas/ && \
cp -f $2/changelog $BINDIR/ && \
cp -f $2/changelog_eng $BINDIR/ && \
cp -f $2/Far.exe.example.ini $BINDIR/ \
) || return 1

return 0
}

function buildfar {
cd $1 || return 1
mkdir -p Include
dos2unix farcolor.hpp
dos2unix plugin.hpp
dos2unix DlgBuilder.hpp
$M4CMD -DINPUT=farcolor.hpp headers.m4 | unix2dos > Include/farcolor.hpp
$M4CMD -DINPUT=plugin.hpp headers.m4 | unix2dos > Include/plugin.hpp
$M4CMD -DINPUT=DlgBuilder.hpp headers.m4 | unix2dos > Include/DlgBuilder.hpp

dos2unix PluginW.pas
dos2unix FarColorW.pas
$M4CMD -DINPUT=PluginW.pas headers.m4 | unix2dos > Include/PluginW.pas
$M4CMD -DINPUT=FarColorW.pas headers.m4 | unix2dos > Include/FarColorW.pas

unix2dos -m changelog
unix2dos -m changelog_eng
unix2dos Far.exe.example.ini

cd ..

(buildfar2 32 $1 && buildfar2 64 $1) || return 1

return 0
}

rm -fR far
( \
	cp -R far.git/far ./ && \
	buildfar far \
) || exit 1


