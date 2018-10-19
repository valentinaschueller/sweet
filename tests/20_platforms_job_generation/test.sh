#! /bin/bash

if [[ -z "$SWEET_ROOT" ]]; then
	echo "SWEET_ROOT environment variable not found"
	exit 1
fi


BACKDIR="$(pwd)"


cd "${SWEET_ROOT}"
PLATFORMS=$(ls -d -1 platforms/??_*)

for PLATFORM  in $PLATFORMS; do

	TEST_PLATFORM_DIR="${PLATFORM/platforms\//}"
	echo_info_hline
	echo_info "Testing platform from directory '$TEST_PLATFORM_DIR'"
	echo_info_hline

	cd "${SWEET_ROOT}"
	. ./local_software/load_platform.sh $TEST_PLATFORM_DIR || exit 1
	cd "${BACKDIR}"

	# Generate some dummy job
	mule.benchmark.cleanup_all || exit 1
	./benchmark_create_job_scripts || exit 1
	mule.benchmark.cleanup_all || exit 1
done


echo_success_hline
echo_success "Job generation successful for all platforms"
echo_success_hline
