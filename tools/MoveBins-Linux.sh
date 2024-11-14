echo "moving files..."

#change this location to the path of your build installation, note you might need sudo to create directories
movedir=./release/

#copy binaries
cp -fr ./jkgalaxies.x86_64 $movedir"jkgalaxies.x86_64"
cp -fr ./jkgalaxiesded.x86_64 $movedir"jkgalaxiesded.x86_64"
cp -fr ./codemp/rd-vanilla/rd-galaxies_x86_64.so $movedir"rd-galaxies_x86_64.so"
cp -fr ./codemp/cgame/cgamex86_64.so $movedir"JKG/cgamex86_64.so"
cp -fr ./codemp/game/gamex86_64.so $movedir"JKG/gamex86_64.so"
cp -fr ./codemp/ui/uix86_64.so $movedir"JKG/uix86_64.so"

#create assets5.pk3 and copy
cd ../JKGalaxies/ && zip -FS -q -r ../build/zz_JKG_Assets5.pk3 * && cd ../build/
cp -fr ./zz_JKG_Assets5.pk3 $movedir"JKG/zz_JKG_Assets5.pk3"
echo "done"
