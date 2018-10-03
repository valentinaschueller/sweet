#! /bin/bash

source ./config_install.sh ""
source ./env_vars.sh ""


echo "*** SHTNS ***"
SRC_LINK="https://www.martin-schreiber.info/pub/sweet_local_software/nschaeff-shtns-2018_10_01.tar.bz2"
FILENAME="`basename $SRC_LINK`"
BASENAME="nschaeff-shtns-2018_10_01"

if [ ! -e "$SWEET_LOCAL_SOFTWARE_DST_DIR/lib/python3.6/site-packages/shtns.py"  -o "$1" != "" ]; then

	cd "$SWEET_LOCAL_SOFTWARE_SRC_DIR"
	download "$SRC_LINK" "$FILENAME" || exit 1

	tar xjf "$FILENAME"
	cd "$BASENAME"

	# Python, no OpenMP
	make clean
	./configure --prefix="$SWEET_LOCAL_SOFTWARE_DST_DIR" --enable-python --disable-openmp || exit 1
	make || exit 1
	python3 setup.py install --prefix="$SWEET_LOCAL_SOFTWARE_DST_DIR"

	# Python, OpenMP
	make clean
	./configure --prefix="$SWEET_LOCAL_SOFTWARE_DST_DIR" --enable-python --enable-openmp || exit 1
	make || exit 1
	python3 setup.py install --prefix="$SWEET_LOCAL_SOFTWARE_DST_DIR"

	echo "DONE"

else
	echo "SHTNS (with python) already installed"
fi
