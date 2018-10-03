#! /bin/bash

source ./config_install.sh ""
source ./env_vars.sh ""


echo "*** GCC5.3 ***"
if [ ! -e "$SWEET_LOCAL_SOFTWARE_DST_DIR/bin/gcc-5.3"  -o "$1" != "" ]; then
	SRC_LINK="https://www.martin-schreiber.info/pub/sweet_local_software/gcc-5.3.0.tar.bz2"
	#SRC_LINK="ftp://ftp.fu-berlin.de/unix/languages/gcc/releases/gcc-5.3.0/gcc-5.3.0.tar.bz2"
	FILENAME="`basename $SRC_LINK`"
	BASENAME="gcc-5.3.0"

	cd "$SWEET_LOCAL_SOFTWARE_SRC_DIR"
	download "$SRC_LINK" "$FILENAME" || exit 1

	echo "Uncompressing $FILENAME"
	tar xjf "$FILENAME" || exit 1

	if [ ! -e "$BASENAME" ]; then
		echo "$BASENAME does not exist"
		exit 1
	fi

	##########################
	# GMP
	##########################
	LIB_LINK="https://gmplib.org/download/gmp/gmp-6.1.0.tar.bz2"
	LIB_FILENAME="`basename $LIB_LINK`"
	LIB_BASENAME="gmp-6.1.0"
	LIB_BASENAME_SHORT="gmp"

	download "$LIB_LINK" "$LIB_FILENAME" || exit 1

	if [ ! -e "$BASENAME/$LIB_BASENAME" ]; then
		tar xjf "$LIB_FILENAME" || exit 1
	fi

	mv "$LIB_BASENAME" "$BASENAME/"
	ln -sf "$LIB_BASENAME" "./$BASENAME/$LIB_BASENAME_SHORT"

	##########################
	# MPFR
	##########################
	LIB_LINK="https://ftp.gnu.org/gnu/mpfr/mpfr-3.1.3.tar.bz2"
	LIB_FILENAME="`basename $LIB_LINK`"
	LIB_BASENAME="mpfr-3.1.3"
	LIB_BASENAME_SHORT="mpfr"
	if [ ! -e "$LIB_FILENAME" ]; then
		echo "Downloading file $LIB_FILENAME"

		download "$LIB_LINK" "$LIB_FILENAME" || exit 1
	else
		echo "Using existing file $LIB_FILENAME"
	fi

	if [ ! -e "$BASENAME/$LIB_BASENAME" ]; then
		tar xjf "$LIB_FILENAME" || exit 1
	fi

	mv "$LIB_BASENAME" "$BASENAME/"
	ln -sf "$LIB_BASENAME" "./$BASENAME/$LIB_BASENAME_SHORT"



	##########################
	# MPFR
	##########################
	LIB_LINK="ftp://ftp.gnu.org/gnu/mpc/mpc-1.0.3.tar.gz"
	LIB_FILENAME="`basename $LIB_LINK`"
	LIB_BASENAME="mpc-1.0.3"
	LIB_BASENAME_SHORT="mpc"
	download "$LIB_LINK" "$LIB_FILENAME" || exit 1
	
	if [ ! -e "$BASENAME/$LIB_BASENAME" ]; then
		tar xzf "$LIB_FILENAME" || exit 1
	fi

	mv "$LIB_BASENAME" "$BASENAME/"
	ln -sf "$LIB_BASENAME" "./$BASENAME/$LIB_BASENAME_SHORT"


	##########################
	# ISL
	##########################
	LIB_LINK="https://isl.gforge.inria.fr/isl-0.15.tar.gz"
	LIB_FILENAME="`basename $LIB_LINK`"
	LIB_BASENAME="isl-0.15"
	LIB_BASENAME_SHORT="isl"
	download "$LIB_LINK" "$LIB_FILENAME" || exit 1

	if [ ! -e "$BASENAME/$LIB_BASENAME" ]; then
		tar xzf "$LIB_FILENAME" || exit 1
	fi

	mv "$LIB_BASENAME" "$BASENAME/"
	ln -sf "$LIB_BASENAME" "./$BASENAME/$LIB_BASENAME_SHORT"

	##############################
	##############################

	cd "$BASENAME"
	
	export CC=gcc
	export CXX=g++
	export LINK=ld
	./configure --disable-multilib --enable-languages=c++,fortran  --prefix="$SWEET_LOCAL_SOFTWARE_DST_DIR" --program-suffix=-5.3 || exit 1

	make || exit 1
	make install || exit 1

	for i in g++ gcc gcc-ar gcc-nm gcc-ranlib gfortran gcov gcov-tool gfortran; do
		ln -sf "$SWEET_LOCAL_SOFTWARE_DST_DIR/bin/$i-5.3" "$DST_DIR/bin/$i"
	done

	echo "DONE"

else
	echo "GCC already installed"
fi
