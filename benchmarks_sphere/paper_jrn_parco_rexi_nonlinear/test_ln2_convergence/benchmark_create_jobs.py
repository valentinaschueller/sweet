#! /usr/bin/env python3

import os
import sys
import math

from itertools import product


from mule.JobGeneration import *
from mule.JobParallelizationDimOptions import *
from mule.JobParallelization import *
p = JobGeneration()

verbose = False
#verbose = True



##################################################

##################################################

p.compile.mode = 'release'
if '_gcc' in os.getenv('MULE_PLATFORM_ID'):
	p.compile.compiler = 'gcc'
else:
	p.compile.compiler = 'intel'
p.compile.sweet_mpi = 'enable'
p.compile.sweet_mpi = 'disable'

p.runtime.space_res_spectral = 128
p.runtime.reuse_plans = -1	# enforce using plans (todo, enforcing not yet implemented)!

p.compile.rexi_timings_additional_barriers = 'disable'
p.compile.rexi_allreduce = 'disable'

p.parallelization.core_oversubscription = False
p.parallelization.core_affinity = 'compact'

p.compile.threading = 'omp'
p.compile.rexi_thread_parallel_sum = 'disable'

params_simtime = []
#params_simtime += [60*60*i for i in range(1, 2)]
#params_simtime += [60*60*i for i in range(1, 24)]
#params_simtime += [60*60*i for i in range(1, 24)]
#params_simtime += [60*60*24*i for i in range(1, 6)]
params_simtime += [60*60*24*i for i in range(1, 2)]

gen_reference_solution = True
timestep_size_reference = 10

#params_timestep_sizes_explicit = [30]
params_timestep_sizes_explicit = [15/16, 15/8, 15/4, 15, 30, 60, 120, 240, 480, 960]

#params_timestep_sizes_implicit = [30]
params_timestep_sizes_implicit = [15/16, 15/8, 15/4, 15, 30, 60, 120, 240, 480, 960]

#params_timestep_sizes_rexi = [30]
params_timestep_sizes_rexi = params_timestep_sizes_implicit[:]



# Parallelization
params_pspace_num_cores_per_rank = [p.platform_resources.num_cores_per_socket]
#params_pspace_num_threads_per_rank = [i for i in range(1, p.platform_resources.num_cores_per_socket+1)]
params_pspace_num_threads_per_rank = [p.platform_resources.num_cores_per_socket]
params_ptime_num_cores_per_rank = [1]

unique_id_filter = []
#unique_id_filter.append('simparams')
unique_id_filter.append('compile')
unique_id_filter.append('disc_space')
unique_id_filter.append('timestep_order')
#unique_id_filter.append('timestep_size')
unique_id_filter.append('rexi_params')
unique_id_filter.append('benchmark')

p.unique_id_filter = unique_id_filter



# No output
#p.runtime.output_filename = "-"

# REXI stuff
params_ci_N = [128]
#params_ci_N = [1]
params_ci_max_imag = [30.0]
params_ci_max_real = [10.0]

#
# Scale the CI circle radius relative to this time step size
# We do this simply to get a consistent time stepping method
# Otherwise, CI would not behave consistently
#
params_ci_max_imag_scaling_relative_to_timestep_size = 480
#params_ci_max_imag_scaling_relative_to_timestep_size = None

##################################################


#
# Force deactivating Turbo mode
#
p.parallelization.force_turbo_off = True


def estimateWallclockTime(p):
	"""
	Return an estimated wallclock time
	"""


	###
	### ALWAYS RETURN 15 MINUTES
	### We're interested in runs which take up to 10 minutes
	###
	return 60*15
	###
	###
	###


	#
	# Reference wallclock time and corresponding time step size
	# e.g. for explicit RK2 integration scheme
	#
	# On Cheyenne with GNU compiler
	# OMP_NUM_THREADS=18
	# 247.378 seconds for ln_erk2 with dt=30 m=128 t=432000
	#
	ref_wallclock_seconds = 60*4
	ref_simtime = 432000
	ref_timestep_size = 60
	ref_mode_res = 128

	# Use this scaling for additional wallclock time
	safety_scaling = 10
	# 5 Min additionaly
	safety_add = 60*5

	wallclock_seconds = ref_wallclock_seconds

	# inv. linear with simulation time
	wallclock_seconds *= p.runtime.max_simulation_time/ref_simtime

	# linear with time step size
	wallclock_seconds *= ref_timestep_size/p.runtime.timestep_size

	# quadratic with resolution
	wallclock_seconds *= pow(ref_mode_res/p.runtime.space_res_spectral, 2.0)

	if p.runtime.rexi_method != '':
		if p.runtime.rexi_method != 'ci':
			raise Exception("TODO: Support other REXI methods")

		# Complex-valued
		wallclock_seconds *= 2.0

		# Number of REXI terms
		wallclock_seconds *= p.runtime.rexi_ci_n

		# Parallelization in time
		wallclock_seconds /= p.parallelization.pardims_dict['time'].num_ranks

	if wallclock_seconds <= 0:
		raise Exception("Estimated wallclock_seconds <= 0")

	wallclock_seconds *= safety_scaling
	wallclock_seconds += safety_add

	if wallclock_seconds > p.platform_resources.max_wallclock_seconds:
		wallclock_seconds = p.platform_resources.max_wallclock_seconds

	return wallclock_seconds

p.compile.lapack = 'enable'
p.compile.mkl = 'disable'

# Request dedicated compile script
p.compilecommand_in_jobscript = False


#
# Run simulation on plane or sphere
#
p.compile.program = 'swe_sphere'

p.compile.plane_spectral_space = 'disable'
p.compile.plane_spectral_dealiasing = 'disable'
p.compile.sphere_spectral_space = 'enable'
p.compile.sphere_spectral_dealiasing = 'enable'

p.compile.benchmark_timings = 'enable'

p.compile.quadmath = 'enable'


#
# Activate Fortran source
#
p.compile.fortran_source = 'enable'


# Verbosity mode
p.runtime.verbosity = 0

#
# Mode and Physical resolution
#
p.runtime.space_res_spectral = 128
p.runtime.space_res_physical = -1

#
# Benchmark
#
p.runtime.benchmark_name = "galewsky"

#
# Compute error
#
p.runtime.compute_error = 0

#
# Preallocate the REXI matrices
#
p.runtime.rexi_sphere_preallocation = 1

# Leave instability checks activated
# Don't activate them since they are pretty costly!!!
p.runtime.instability_checks = 0


#
# REXI method
# N=64, SX,SY=50 and MU=0 with circle primitive provide good results
#
p.runtime.rexi_method = ''
p.runtime.rexi_ci_n = 128
p.runtime.rexi_ci_max_real = -999
p.runtime.rexi_ci_max_imag = -999
p.runtime.rexi_ci_sx = -1
p.runtime.rexi_ci_sy = -1
p.runtime.rexi_ci_mu = 0
p.runtime.rexi_ci_primitive = 'circle'

#p.runtime.rexi_beta_cutoff = 1e-16
#p.runtime.rexi_beta_cutoff = 0

p.runtime.viscosity = 0.0



p.runtime.sphere_extended_modes = 0

# Groups to execute, see below
# l: linear
# ln: linear and nonlinear
#groups = ['l1', 'l2', 'ln1', 'ln2', 'ln4']
groups = ['ln2']





#
# allow including this file
#
if __name__ == "__main__":

	if len(sys.argv) > 1:
		groups = [sys.argv[1]]

	print("Groups: "+str(groups))

	for group in groups:
		# 1st order linear

		# 2nd order nonlinear
		if group == 'ln2':
			ts_methods = [
				['ln_erk',		4,	4,	0],	# reference solution

				###########
				# Runge-Kutta
				###########
				['ln_erk',		2,	2,	0],

				###########
				# CN
				###########
#				['lg_irk_lc_n_erk_ver0',	2,	2,	0],
				['lg_irk_lc_n_erk_ver1',	2,	2,	0],

#				['l_irk_n_erk_ver0',	2,	2,	0],
#				['l_irk_n_erk_ver1',	2,	2,	0],

				###########
				# REXI
				###########
#				['lg_rexi_lc_n_erk_ver0',	2,	2,	0],
#				['lg_rexi_lc_n_erk_ver1',	2,	2,	0],

#				['l_rexi_n_erk_ver0',	2,	2,	0],
#				['l_rexi_n_erk_ver1',	2,	2,	0],

				###########
				# ETDRK
				###########
#				['lg_rexi_lc_n_etdrk',	2,	2,	0],
#				['l_rexi_n_etdrk',	2,	2,	0],
			]


		# 4th order nonlinear
		if group == 'ln4':
			ts_methods = [
				['ln_erk',		4,	4,	0],	# reference solution
				['l_rexi_n_etdrk',	4,	4,	0],
				['ln_erk',		4,	4,	0],
			]



		for p.runtime.max_simulation_time in params_simtime:

			p.runtime.output_timestep_size = p.runtime.max_simulation_time

			#
			# Reference solution
			#
			if gen_reference_solution:
				tsm = ts_methods[0]
				p.runtime.timestep_size  = timestep_size_reference

				p.runtime.timestepping_method = tsm[0]
				p.runtime.timestepping_order = tsm[1]
				p.runtime.timestepping_order2 = tsm[2]

				# Update TIME parallelization
				ptime = JobParallelizationDimOptions('time')
				ptime.num_cores_per_rank = 1
				ptime.num_threads_per_rank = 1 #pspace.num_cores_per_rank
				ptime.num_ranks = 1

				pspace = JobParallelizationDimOptions('space')
				pspace.num_cores_per_rank = 1
				pspace.num_threads_per_rank = params_pspace_num_cores_per_rank[-1]
				pspace.num_ranks = 1

				# Setup parallelization
				p.setup_parallelization([pspace, ptime])

				if verbose:
					pspace.print()
					ptime.print()
					p.parallelization.print()

				if len(tsm) > 4:
					s = tsm[4]
					p.load_from_dict(tsm[4])

				p.parallelization.max_wallclock_seconds = estimateWallclockTime(p)

				p.gen_jobscript_directory('job_benchref_'+p.getUniqueID())

				p.reference_job_unique_id = p.job_unique_id

			#
			# Create job scripts
			#
			for tsm in ts_methods[1:]:
				p.runtime.timestepping_method = tsm[0]
				p.runtime.timestepping_order = tsm[1]
				p.runtime.timestepping_order2 = tsm[2]

				if len(tsm) > 4:
					s = tsm[4]
					p.runtime.load_from_dict(tsm[4])

				tsm_name = tsm[0]
				if 'ln_erk' in tsm_name:
					params_timestep_sizes = params_timestep_sizes_explicit
				elif 'l_erk' in tsm_name or 'lg_erk' in tsm_name:
					params_timestep_sizes = params_timestep_sizes_explicit
				elif 'l_irk' in tsm_name or 'lg_irk' in tsm_name:
					params_timestep_sizes = params_timestep_sizes_implicit
				elif 'l_rexi' in tsm_name or 'lg_rexi' in tsm_name:
					params_timestep_sizes = params_timestep_sizes_rexi
				else:
					print("Unable to identify time stepping method "+tsm_name)
					sys.exit(1)


				for pspace_num_cores_per_rank, pspace_num_threads_per_rank, p.runtime.timestep_size in product(params_pspace_num_cores_per_rank, params_pspace_num_threads_per_rank, params_timestep_sizes):
					pspace = JobParallelizationDimOptions('space')
					pspace.num_cores_per_rank = pspace_num_cores_per_rank
					pspace.num_threads_per_rank = pspace_num_threads_per_rank
					pspace.num_ranks = 1
					pspace.setup()

					if not '_rexi' in p.runtime.timestepping_method:
						p.runtime.rexi_method = ''

						# Update TIME parallelization
						ptime = JobParallelizationDimOptions('time')
						ptime.num_cores_per_rank = 1
						ptime.num_threads_per_rank = 1 #pspace.num_cores_per_rank
						ptime.num_ranks = 1
						ptime.setup()

						p.setup_parallelization([pspace, ptime])

						if verbose:
							pspace.print()
							ptime.print()
							p.parallelization.print()

						p.parallelization.max_wallclock_seconds = estimateWallclockTime(p)

						p.gen_jobscript_directory('job_bench_'+p.getUniqueID())

					else:

						for ci_N, ci_max_imag, ci_max_real in product(params_ci_N, params_ci_max_imag, params_ci_max_real):

							if params_ci_max_imag_scaling_relative_to_timestep_size != None:
								ci_max_imag *= (p.runtime.timestep_size/params_ci_max_imag_scaling_relative_to_timestep_size)

							p.runtime.load_from_dict({
								'rexi_method': 'ci',
								'ci_n':ci_N,
								'ci_max_real':ci_max_real,
								'ci_max_imag':ci_max_imag,
								'half_poles':0,
								#'ci_gaussian_filter_scale':0.0,
								#'ci_gaussian_filter_dt_norm':0.0,	# unit scaling for T128 resolution
								#'ci_gaussian_filter_exp_N':0.0,
							})

							time_ranks = ci_N
							time_ranks = 8

							#for time_ranks in params_ptime_num_cores_per_rank:
							if True:

								# Update TIME parallelization
								ptime = JobParallelizationDimOptions('time')
								ptime.num_cores_per_rank = 1
								ptime.num_threads_per_rank = 1
								ptime.num_ranks = time_ranks
								ptime.setup()

								p.setup_parallelization([pspace, ptime])

								if verbose:
									pspace.print()
									ptime.print()
									p.parallelization.print()

								# Generate only scripts with max number of cores
								p.parallelization.max_wallclock_seconds = estimateWallclockTime(p)

								if int(p.runtime.max_simulation_time / p.runtime.timestep_size) * p.runtime.timestep_size != p.runtime.max_simulation_time:
									raise Exception("Simtime "+str(p.runtime.max_simulation_time)+" not dividable without remainder by "+str(p.runtime.timestep_size))

								p.gen_jobscript_directory('job_bench_'+p.getUniqueID())

	if p.runtime.reuse_plans > 0:
		#
		# SHTNS plan generation scripts
		#
		p.runtime.reuse_plans = 1	# search for awesome plans and store them

		#
		# Create dummy scripts to be used for SHTNS script generation
		#

		# No parallelization in time
		ptime = JobParallelizationDimOptions('time')
		ptime.num_cores_per_rank = 1
		ptime.num_threads_per_rank = 1
		ptime.num_ranks = 1
		ptime.setup()

		for tsm in ts_methods[1:2]:

			p.runtime.timestepping_method = tsm[0]
			p.runtime.timestepping_order = tsm[1]
			p.runtime.timestepping_order2 = tsm[2]

			if not '_rexi' in p.runtime.timestepping_method:
				p.runtime.rexi_method = ''
			else:
				p.runtime.rexi_method = 'ci'

			if len(tsm) > 4:
				s = tsm[4]
				p.runtime.load_from_dict(tsm[4])


			for pspace_num_cores_per_rank, pspace_num_threads_per_rank, p.runtime.timestep_size in product(params_pspace_num_cores_per_rank, params_pspace_num_threads_per_rank, [params_timestep_sizes_explicit[0]]):
				pspace = JobParallelizationDimOptions('space')
				pspace.num_cores_per_rank = pspace_num_cores_per_rank
				pspace.num_threads_per_rank = pspace_num_threads_per_rank
				pspace.num_ranks = 1
				pspace.setup()

				p.setup_parallelization([pspace, ptime])

				# Use 10 minutes per default to generate plans
				p.parallelization.max_wallclock_seconds = 60*10

				# Set simtime to 0
				p.runtime.max_simulation_time = 0

				# No output
				p.runtime.output_timestep_size = -1
				p.runtime.output_filename = "-"

				jobdir = 'job_plan_'+p.getUniqueID()
				p.gen_jobscript_directory(jobdir)



	# Write compile script
	p.write_compilecommands("./compile_platform_"+p.platforms.platform_id+".sh")


