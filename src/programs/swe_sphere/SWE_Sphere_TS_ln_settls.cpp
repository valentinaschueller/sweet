/*
 * SWE_Sphere_TS_ln_settls.cpp
 *
 *  Created on: 24 Sep 2019
 *      Author: Martin Schreiber <SchreiberX@gmail.com>
 *
 *  Changelog:
 *  	2019-10-24: Partly based on plane version
 */

#include "SWE_Sphere_TS_ln_settls.hpp"
#include <sweet/sphere/SphereData_DebugContainer.hpp>



void SWE_Sphere_TS_ln_settls::run_timestep_pert(
		SphereData_Spectral &io_phi_pert,	///< prognostic variables
		SphereData_Spectral &io_vrt,	///< prognostic variables
		SphereData_Spectral &io_div,	///< prognostic variables

		double i_fixed_dt,			///< if this value is not equal to 0, use this time step size instead of computing one
		double i_simulation_timestamp
)
{
	if (timestepping_order == 1)
	{
		FatalError("TODO run_timestep_1st_order_pert");
		//run_timestep_1st_order_pert(io_phi_pert, io_vrt, io_div, i_fixed_dt, i_simulation_timestamp);
	}
	else if (timestepping_order == 2)
	{
		run_timestep_2nd_order_pert(io_phi_pert, io_vrt, io_div, i_fixed_dt, i_simulation_timestamp);
	}
	else
	{
		FatalError("Only orders 1/2 supported (ERRID 098nd89eje)");
	}
}




void SWE_Sphere_TS_ln_settls::run_timestep_2nd_order_pert(
	SphereData_Spectral &io_U_phi_pert,	///< prognostic variables
	SphereData_Spectral &io_U_vrt,		///< prognostic variables
	SphereData_Spectral &io_U_div,		///< prognostic variables

	double i_dt,					///< if this value is not equal to 0, use this time step size instead of computing one
	double i_simulation_timestamp
)
{
	const SphereData_Config *sphereDataConfig = io_U_phi_pert.sphereDataConfig;
//	double dt_div_radius = i_dt/simVars.sim.sphere_radius;

	const SphereData_Spectral &U_phi_pert = io_U_phi_pert;
	const SphereData_Spectral &U_vrt = io_U_vrt;
	const SphereData_Spectral &U_div = io_U_div;

	if (i_simulation_timestamp == 0)
	{
		/*
		 * First time step:
		 * Simply backup existing fields for multi-step parts of this algorithm.
		 * TODO: Maybe we should replace this with some 1st order SL time step or some subcycling
		 */
		U_phi_pert_prev = U_phi_pert;
		U_vrt_prev = U_vrt;
		U_div_prev = U_div;
	}


	/*
	 * Step 1) SL
	 * Compute Lagrangian trajectories based on SETTLS.
	 * This depends on V(t-\Delta t) and V(t).
	 *
	 * See Hortal's paper for equation.
	 */

	SphereData_Physical U_u_lon_prev(sphereDataConfig);
	SphereData_Physical U_v_lat_prev(sphereDataConfig);
	op.vortdiv_to_uv(U_vrt_prev, U_div_prev, U_u_lon_prev, U_v_lat_prev, false);

	SphereData_Physical U_u(sphereDataConfig);
	SphereData_Physical U_v(sphereDataConfig);
	op.vortdiv_to_uv(U_vrt, U_div, U_u, U_v, false);

	// Initialize departure points with arrival points for iterative corrections
	ScalarDataArray pos_lon_d(sphereDataConfig->physical_array_data_number_of_elements);
	ScalarDataArray pos_lat_d(sphereDataConfig->physical_array_data_number_of_elements);


	// Calculate departure points
	semiLagrangian.semi_lag_departure_points_settls(
			U_u_lon_prev, U_v_lat_prev,
			U_u, U_v,

			i_dt,
			i_simulation_timestamp,
			simVars.sim.sphere_radius,
			nullptr,

			pos_lon_d, pos_lat_d,		// OUTPUT

			op,

			simVars.disc.timestepping_order,
			simVars.disc.semi_lagrangian_max_iterations,
			simVars.disc.semi_lagrangian_convergence_threshold,
			simVars.disc.semi_lagrangian_approximate_sphere_geometry,
			simVars.disc.semi_lagrangian_interpolation_limiter && false
	);

	/*
	 * Step 2) Midpoint rule
	 * Put everything together with midpoint rule and solve resulting Helmholtz problem
	 */

	/*
	 * Step 2a) Compute RHS
	 * R = X_D + 1/2 dt L_D + dt N*
	 */

	/*
	 * Compute X_D
	 */
	SphereData_Spectral U_phi_pert_D;
	SphereData_Spectral U_vrt_D;
	SphereData_Spectral U_div_D;

	if (coriolis_treatment != CORIOLIS_SEMILAGRANGIAN)
	{
		if (nonlinear_advection_treatment == NL_ADV_SEMILAGRANGIAN)
		{
			SphereData_Physical U_phi_pert_D_phys(sphereDataConfig);
			sphereSampler.bicubic_scalar(
					U_phi_pert.getSphereDataPhysical(),
					pos_lon_d, pos_lat_d,
					U_phi_pert_D_phys,
					false,
					simVars.disc.semi_lagrangian_interpolation_limiter,
					simVars.disc.semi_lagrangian_sampler_use_pole_pseudo_points
				);
			U_phi_pert_D = U_phi_pert_D_phys;

#if 0
			/*
			 * DO NOT USE VRT/DIV!!!
			 * THIS DOESN'T WORK!!!
			 */

			SphereData_Physical U_vort_D_phys(sphereDataConfig);
			sphereSampler.bicubic_scalar(U_vrt.getSphereDataPhysical(), pos_lon_d, pos_lat_d, U_vort_D_phys, false, simVars.disc.semi_lagrangian_interpolation_limiter);
			U_vrt_D = U_vort_D_phys;

			SphereData_Physical U_div_D_phys(sphereDataConfig);
			sphereSampler.bicubic_scalar(U_div.getSphereDataPhysical(), pos_lon_d, pos_lat_d, U_div_D_phys, false, simVars.disc.semi_lagrangian_interpolation_limiter);
			U_div_D = U_div_D_phys;

#else

			SphereData_Physical U_u_D_phys(sphereDataConfig);
			sphereSampler.bicubic_scalar(
					U_u,
					pos_lon_d, pos_lat_d,
					U_u_D_phys,
					true,
					simVars.disc.semi_lagrangian_interpolation_limiter,
					simVars.disc.semi_lagrangian_sampler_use_pole_pseudo_points
				);

			SphereData_Physical U_v_D_phys(sphereDataConfig);
			sphereSampler.bicubic_scalar(
					U_v,
					pos_lon_d, pos_lat_d,
					U_v_D_phys,
					true,
					simVars.disc.semi_lagrangian_interpolation_limiter,
					simVars.disc.semi_lagrangian_sampler_use_pole_pseudo_points
				);

			U_vrt_D.setup(sphereDataConfig);
			U_div_D.setup(sphereDataConfig);
			op.uv_to_vortdiv(U_u_D_phys, U_v_D_phys, U_vrt_D, U_div_D, false);

#endif

		}
		else
		{
			U_phi_pert_D = U_phi_pert;
			U_vrt_D = U_vrt;
			U_div_D = U_div;
		}
	}
	else
	{
		if (nonlinear_advection_treatment == NL_ADV_SEMILAGRANGIAN)
			FatalError("Only Coriolis semi-lagrangian advection is not supported");

		FatalError("TODO: Fix me!!!");

		SphereData_Spectral U_vort_D(sphereDataConfig);
		SphereData_Spectral U_div_D(sphereDataConfig);

		/*
		 * Prepare SL Coriolis treatment
		 */
		SphereData_Physical sl_coriolis_arrival;
		SphereData_Physical sl_coriolis_departure;
		if (coriolis_treatment == CORIOLIS_SEMILAGRANGIAN)
		{
			sl_coriolis_arrival.setup(sphereDataConfig);

			/*
			 * Coriolis term
			 * 		2 \Omega r
			 * at arrival points
			 */
			sl_coriolis_arrival.physical_update_lambda(
				[&](double lon, double lat, double &io_data)
				{
					io_data = std::sin(lat)*2.0*simVars.sim.sphere_rotating_coriolis_omega*simVars.sim.sphere_radius;
				}
			);

			/*
			 * Coriolis term
			 * 		2 \Omega r
			 * at departure points
			 */
			sl_coriolis_departure = Convert_ScalarDataArray_to_SphereDataPhysical::convert(pos_lat_d, sphereDataConfig);
			sl_coriolis_departure.physical_update_lambda(
				[&](double lon, double lat, double &io_data)
				{
					io_data = std::sin(io_data)*2.0*simVars.sim.sphere_rotating_coriolis_omega*simVars.sim.sphere_radius;
				}
			);
		}


		if (coriolis_treatment != CORIOLIS_SEMILAGRANGIAN)
		{
			FatalError("TODO: Make sure this is correct!");

			SphereData_Physical U_vort_D_phys(sphereDataConfig);
			SphereData_Physical U_div_D_phys(sphereDataConfig);

			sphereSampler.bicubic_scalar(
					U_vrt.getSphereDataPhysical(),
					pos_lon_d, pos_lat_d,
					U_vort_D_phys,
					false,
					simVars.disc.semi_lagrangian_interpolation_limiter,
					simVars.disc.semi_lagrangian_sampler_use_pole_pseudo_points
				);
			sphereSampler.bicubic_scalar(
					U_div.getSphereDataPhysical(),
					pos_lon_d, pos_lat_d,
					U_div_D_phys,
					false,
					simVars.disc.semi_lagrangian_interpolation_limiter,
					simVars.disc.semi_lagrangian_sampler_use_pole_pseudo_points
				);

			U_vort_D.loadSphereDataPhysical(U_vort_D_phys);
			U_div_D.loadSphereDataPhysical(U_div_D_phys);
		}
		else
		{
			/*
			 * TODO:
			 * It seems that using sl_coriolis_departure/arrival
			 * in spectral space leads to artificial high frequency modes!!!
			 */

			FatalError("TODO: Get this running!");

			SphereData_Physical U_u_D_phys(sphereDataConfig);
			SphereData_Physical U_v_D_phys(sphereDataConfig);

			/*
			 * Do this in physical space, since there would be numerical oscillations if applying the sl_coriolis_arrival/depature in spectral space
			 */
			sphereSampler.bicubic_scalar(
					U_u,
					pos_lon_d,
					pos_lat_d,
					U_u_D_phys,
					true,
					simVars.disc.semi_lagrangian_interpolation_limiter,
					simVars.disc.semi_lagrangian_sampler_use_pole_pseudo_points
				);
			sphereSampler.bicubic_scalar(
					U_v,
					pos_lon_d,
					pos_lat_d,
					U_v_D_phys,
					true,
					simVars.disc.semi_lagrangian_interpolation_limiter,
					simVars.disc.semi_lagrangian_sampler_use_pole_pseudo_points
				);

			U_u_D_phys += sl_coriolis_departure;
			U_u_D_phys -= sl_coriolis_arrival;

			op.uv_to_vortdiv(
					U_u_D_phys, U_v_D_phys,
					U_vort_D, U_div_D,
					false
				);
		}
	}

	/*
	 * Compute L_D
	 */

	SphereData_Spectral L_phi_pert_D(sphereDataConfig);
	SphereData_Spectral L_vort_D(sphereDataConfig);
	SphereData_Spectral L_div_D(sphereDataConfig);

	if (original_linear_operator_sl_treatment)
	{
		/*
		 * Method 1) First evaluate L, then sample result at departure point
		 */
		SphereData_Spectral L_phi_pert(sphereDataConfig);
		SphereData_Spectral L_vort(sphereDataConfig);
		SphereData_Spectral L_div(sphereDataConfig);

		if (coriolis_treatment == CORIOLIS_LINEAR)
		{
			swe_sphere_ts_l_erk->euler_timestep_update(U_phi_pert, U_vrt, U_div, L_phi_pert, L_vort, L_div, i_simulation_timestamp);
		}
		else
		{
			swe_sphere_ts_lg_erk->euler_timestep_update(U_phi_pert, U_vrt, U_div, L_phi_pert, L_vort, L_div, i_simulation_timestamp);
		}

		SphereData_Physical L_phi_pert_D_phys(sphereDataConfig);
		sphereSampler.bicubic_scalar(L_phi_pert.getSphereDataPhysical(), pos_lon_d, pos_lat_d, L_phi_pert_D_phys, false, simVars.disc.semi_lagrangian_interpolation_limiter, simVars.disc.semi_lagrangian_sampler_use_pole_pseudo_points);
		L_phi_pert_D.loadSphereDataPhysical(L_phi_pert_D_phys);

#if 0

		SphereData_Physical L_vort_D_phys(sphereDataConfig);
		sphereSampler.bicubic_scalar(L_vort.getSphereDataPhysical(), pos_lon_d, pos_lat_d, L_vort_D_phys, false, simVars.disc.semi_lagrangian_interpolation_limiter);
		L_vort_D.loadSphereDataPhysical(L_vort_D_phys);

		SphereData_Physical L_div_D_phys(sphereDataConfig);
		sphereSampler.bicubic_scalar(L_div.getSphereDataPhysical(), pos_lon_d, pos_lat_d, L_div_D_phys, false, simVars.disc.semi_lagrangian_interpolation_limiter);
		L_div_D.loadSphereDataPhysical(L_div_D_phys);

#else

		SphereData_Physical U_L_u_phys(sphereDataConfig);
		SphereData_Physical U_L_v_phys(sphereDataConfig);
		op.vortdiv_to_uv(L_vort, L_div, U_L_u_phys, U_L_v_phys, false);

		SphereData_Physical U_u_D_phys(sphereDataConfig);
		sphereSampler.bicubic_scalar(U_L_u_phys, pos_lon_d, pos_lat_d, U_u_D_phys, true, simVars.disc.semi_lagrangian_interpolation_limiter, simVars.disc.semi_lagrangian_sampler_use_pole_pseudo_points);

		SphereData_Physical U_v_D_phys(sphereDataConfig);
		sphereSampler.bicubic_scalar(U_L_v_phys, pos_lon_d, pos_lat_d, U_v_D_phys, true, simVars.disc.semi_lagrangian_interpolation_limiter, simVars.disc.semi_lagrangian_sampler_use_pole_pseudo_points);

		op.uv_to_vortdiv(U_u_D_phys, U_v_D_phys, L_vort_D, L_div_D, false);

#endif
	}
	else
	{
		/*
		 * Method 2) First get variables on departure points, then evaluate L
		 */

		SphereData_Physical U_phi_pert_D_phys(sphereDataConfig);
		sphereSampler.bicubic_scalar(U_phi_pert.getSphereDataPhysical(), pos_lon_d, pos_lat_d, U_phi_pert_D_phys, false, simVars.disc.semi_lagrangian_interpolation_limiter, simVars.disc.semi_lagrangian_sampler_use_pole_pseudo_points);
		SphereData_Spectral U_phi_pert_D(sphereDataConfig);
		U_phi_pert_D.loadSphereDataPhysical(U_phi_pert_D_phys);

		SphereData_Physical U_vort_D_phys(sphereDataConfig);
		SphereData_Physical U_div_D_phys(sphereDataConfig);

		FatalError("Get this running with U/V");

		sphereSampler.bicubic_scalar(U_vrt.getSphereDataPhysical(), pos_lon_d, pos_lat_d, U_vort_D_phys, false, simVars.disc.semi_lagrangian_interpolation_limiter, simVars.disc.semi_lagrangian_sampler_use_pole_pseudo_points);
		sphereSampler.bicubic_scalar(U_div.getSphereDataPhysical(), pos_lon_d, pos_lat_d, U_div_D_phys, false, simVars.disc.semi_lagrangian_interpolation_limiter, simVars.disc.semi_lagrangian_sampler_use_pole_pseudo_points);

		SphereData_Spectral U_vort_D(sphereDataConfig);
		SphereData_Spectral U_div_D(sphereDataConfig);

		U_vort_D.loadSphereDataPhysical(U_vort_D_phys);
		U_div_D.loadSphereDataPhysical(U_div_D_phys);

		if (coriolis_treatment == CORIOLIS_LINEAR)
			swe_sphere_ts_l_erk->euler_timestep_update(U_phi_pert_D, U_vort_D, U_div_D, L_phi_pert_D, L_vort_D, L_div_D, i_simulation_timestamp);
		else
			swe_sphere_ts_lg_erk->euler_timestep_update(U_phi_pert_D, U_vort_D, U_div_D, L_phi_pert_D, L_vort_D, L_div_D, i_simulation_timestamp);
	}

	/*
	 * Compute R = X_D + 1/2 dt L_D + dt N*
	 */

	SphereData_Spectral R_phi_pert(sphereDataConfig);
	SphereData_Spectral R_vrt(sphereDataConfig);
	SphereData_Spectral R_div(sphereDataConfig);

	R_phi_pert = U_phi_pert_D + 0.5 * i_dt * L_phi_pert_D;
	R_vrt = U_vrt_D + 0.5 * i_dt * L_vort_D;
	R_div = U_div_D + 0.5 * i_dt * L_div_D;

	/*
	 * Now we care about dt*N*
	 */
	if (nonlinear_divergence_treatment == NL_DIV_NONLINEAR)
	{
		/**
		 * Compute non-linear term N*
		 *
		 * Extrapolate non-linear terms with the equation
		 * N*(t+0.5dt) = 1/2 [ 2*N(t) - N(t-dt) ]_D + N^t
		 *
		 * Here we assume that
		 * 		N = -Phi' * Div (U)
		 * with non-constant U!
		 *
		 * The divergence is already
		 */

		SphereData_Spectral N_phi_t(sphereDataConfig);
		SphereData_Spectral N_phi_prev(sphereDataConfig);

		/*
		 * Compute N = - Phi' * Div (U)
		 */

		/*
		 * See documentation in [sweet]/doc/swe/swe_sphere_formulation/
		 */
		/*
		 * Step SLPHI a
		 * Step SLPHI b
		 */

		// Compute N(t)
		N_phi_t.loadSphereDataPhysical((-(U_phi_pert)).getSphereDataPhysical() * U_div.getSphereDataPhysical());

		// Compute N(t-dt)
		N_phi_prev.loadSphereDataPhysical((-(U_phi_pert_prev)).getSphereDataPhysical() * U_div_prev.getSphereDataPhysical());

		// [ 2*N(t) - N(t-dt) ]_D
		SphereData_Physical N_D_phys(sphereDataConfig);
		sphereSampler.bicubic_scalar(
				(2.0 * N_phi_t - N_phi_prev).getSphereDataPhysical(),	// field to sample
				pos_lon_d, pos_lat_d,
				N_D_phys,
				false,
				simVars.disc.semi_lagrangian_interpolation_limiter,
				simVars.disc.semi_lagrangian_sampler_use_pole_pseudo_points
			);

		// N_D spectral
		SphereData_Spectral N_D(N_D_phys);

		// Compute N*(t+0.5dt) = 1/2 ([ 2*N(t) - N(t-dt) ]_D + N^t)
		SphereData_Spectral N_star = 0.5 * (N_D + N_phi_t);

		// add nonlinear divergence
		R_phi_pert += i_dt * N_star;
	}

	if (coriolis_treatment == CORIOLIS_NONLINEAR)
	{
		/**
		 * Apply Coriolis Effect in physical VELOCITY space
		 *
		 * This is require to stay compatible to all other things
		 */

		/***************************************************************
		 * Calculate f_vort_t and f_div_t caused by Coriolis effect
		 *
		 * See /doc/swe/swe_sphere_formulation, lc_erk
		 */
		/*
		 * Step 1a
		 */
		// u_lon and u_lat already available in physical space
		/*
		 * Step 1b
		 */
		SphereData_Physical ufg = U_u * op.fg;
		SphereData_Physical vfg = U_v * op.fg;

		/*
		 * Step 1c
		 */
		SphereData_Spectral f_vort_t(sphereDataConfig);
		SphereData_Spectral f_div_t(sphereDataConfig);
		op.uv_to_vortdiv(ufg, vfg, f_div_t, f_vort_t, false);

		/*
		 * Step 1d
		 */
		f_vort_t *= -1.0;

		/*
		 * Step 1e
		 */
		// f_div_t already assigned

		/***************************************************************
		 * Calculate f_vort_t_prev and f_div_t_prev caused by Coriolis effect
		 *
		 * See /doc/swe/swe_sphere_formulation, lc_erk
		 */

		/*
		 * Step 1b
		 */

		// u_lon and u_lat already available in physical space
		SphereData_Physical ufg_prev = U_u_lon_prev * op.fg;
		SphereData_Physical vfg_prev = U_v_lat_prev * op.fg;

		/*
		 * Step 1c
		 */
		SphereData_Spectral f_vort_t_prev(sphereDataConfig);
		SphereData_Spectral f_div_t_prev(sphereDataConfig);
		op.uv_to_vortdiv(ufg_prev, vfg_prev, f_div_t_prev, f_vort_t_prev, false);


		/*
		 * Step 1d
		 */
		f_vort_t_prev *= -1.0;

		/*
		 * Step 1e
		 */
		// f_div_t_prev already assigned

		/*******************************************************************
		 * Extrapolation of Coriolis effect
		 */

#if 0

		/*
		 * Extrapolation for vorticity
		 */
		// [ 2*N(t) - N(t-dt) ]_D
		SphereData_Physical f_vort_t_D_phys(sphereDataConfig);
		sphereSampler.bicubic_scalar(
				(2.0 * f_vort_t - f_vort_t_prev).getSphereDataPhysical(),	// field to sample
				pos_lon_d, pos_lat_d,
				f_vort_t_D_phys,
				false,
				simVars.disc.semi_lagrangian_interpolation_limiter
			);

		/*
		 * Extrapolation for divergence
		 */
		// [ 2*N(t) - N(t-dt) ]_D
		SphereData_Physical f_div_t_D_phys(sphereDataConfig);
		sphereSampler.bicubic_scalar(
				(2.0 * f_div_t - f_div_t_prev).getSphereDataPhysical(),	// field to sample
				pos_lon_d, pos_lat_d,
				f_div_t_D_phys,
				false,
				simVars.disc.semi_lagrangian_interpolation_limiter
			);

		// vort_D physical
		SphereData_Spectral f_vrt_t_D(f_vort_t_D_phys);

		// div_D physical
		SphereData_Spectral f_div_t_D(f_div_t_D_phys);

#else

		SphereData_Physical f_u_t(sphereDataConfig);
		SphereData_Physical f_v_t(sphereDataConfig);

		op.vortdiv_to_uv(
				2.0 * f_vort_t - f_vort_t_prev,
				2.0 * f_div_t - f_div_t_prev,
				f_u_t,
				f_v_t,
				false
		);

		SphereData_Physical f_u_t_D(sphereDataConfig);
		SphereData_Physical f_v_t_D(sphereDataConfig);

		sphereSampler.bicubic_scalar(
				f_u_t,
				pos_lon_d, pos_lat_d,
				f_u_t_D,
				false,
				simVars.disc.semi_lagrangian_interpolation_limiter,
				simVars.disc.semi_lagrangian_sampler_use_pole_pseudo_points
			);

		sphereSampler.bicubic_scalar(
				f_v_t,
				pos_lon_d, pos_lat_d,
				f_v_t_D,
				false,
				simVars.disc.semi_lagrangian_interpolation_limiter,
				simVars.disc.semi_lagrangian_sampler_use_pole_pseudo_points
			);


		SphereData_Spectral f_vrt_t_D(sphereDataConfig);
		SphereData_Spectral f_div_t_D(sphereDataConfig);
		op.uv_to_vortdiv(
				f_u_t_D,
				f_v_t_D,
				f_vrt_t_D,
				f_div_t_D,
				false
		);
#endif

		// Compute N*(t+0.5dt) = 1/2 ([ 2*N(t) - N(t-dt) ]_D + N^t)
		SphereData_Spectral f_vort_t_star = 0.5 * (f_vrt_t_D + f_vort_t);

		// add nonlinear vorticity
		R_vrt += i_dt * f_vort_t_star;



		// Compute N*(t+0.5dt) = 1/2 ([ 2*N(t) - N(t-dt) ]_D + N^t)
		SphereData_Spectral f_div_t_star = 0.5 * (f_div_t_D + f_div_t);

		// add nonlinear divergence
		R_div += i_dt * f_div_t_star;
	}

	/*
	 * Step 2b) Solve Helmholtz problem
	 * X - 1/2 dt LX = R
	 */
	if (linear_treatment == LINEAR_IMPLICIT)
	{
		if (coriolis_treatment == CORIOLIS_LINEAR)
		{
			swe_sphere_ts_l_irk->run_timestep_nonpert(
					R_phi_pert, R_vrt, R_div,
					0.5 * i_dt,
					i_simulation_timestamp
				);
		}
		else
		{
			swe_sphere_ts_lg_irk->run_timestep_nonpert(
					R_phi_pert, R_vrt, R_div,
					0.5 * i_dt,
					i_simulation_timestamp
				);
		}
	}
	else if (linear_treatment == LINEAR_EXPONENTIAL)
	{
		swe_sphere_ts_l_rexi->run_timestep_nonpert(
			R_phi_pert, R_vrt, R_div,
			0.5 * i_dt,
			i_simulation_timestamp
		);
	}

	/*
	 * Backup previous variables for multi-step SL method
	 */
	U_phi_pert_prev = U_phi_pert;
	U_vrt_prev = U_vrt;
	U_div_prev = U_div;

	/*
	 * Return new fields stored in R_*
	 */
	io_U_phi_pert = R_phi_pert;
	io_U_vrt = R_vrt;
	io_U_div = R_div;
}




/**
 * SETTLS implementation (see Hortal 2002)
 *
 * Solve SWE with Crank-Nicolson implicit time stepping
 * (spectral formulation for Helmholtz equation) with semi-Lagrangian
 * SL-SI-SP
 *
 * U_t = L U(0)
 *
 * Fully implicit version:
 *
 * (U(tau) - U(0)) / tau = 0.5*(L U(tau) + L U(0))
 *
 * <=> U(tau) - U(0) =  tau * 0.5*(L U(tau) + L U(0))
 *
 * <=> U(tau) - 0.5* L tau U(tau) = U(0) + tau * 0.5*L U(0)
 *
 * <=> (1 - 0.5 L tau) U(tau) = (1 + tau*0.5*L) U(0)
 *
 * <=> (2/tau - L) U(tau) = (2/tau + L) U(0)
 *
 * <=> U(tau) = (2/tau - L)^{-1} (2/tau + L) U(0)
 *
 * Semi-implicit has Coriolis term as totally explicit
 *
 * Semi-Lagrangian:
 *   U(tau) is on arrival points
 *   U(0) is on departure points
 *
 * Nonlinear term is added following Hortal (2002)
 * http://onlinelibrary.wiley.com/doi/10.1002/qj.200212858314/pdf
 */

void SWE_Sphere_TS_ln_settls::run_timestep_1st_order(
		SphereData_Spectral &io_phi,	///< prognostic variables
		SphereData_Spectral &io_vrt,	///< prognostic variables
		SphereData_Spectral &io_div,	///< prognostic variables

		double i_dt,					///< if this value is not equal to 0, use this time step size instead of computing one
		double i_simulation_timestamp)
{
	const SphereData_Config *sphereDataConfig = io_phi.sphereDataConfig;
	double gh = simVars.sim.gravitation * simVars.sim.h0;

//	double dt_radius = i_dt/simVars.sim.sphere_radius;

	if (i_dt <= 0)
		FatalError("SWE_Sphere_TS_ln_settls: Only constant time step size allowed (Please set --dt)");

	if (i_simulation_timestamp == 0)
	{
		/*
		 * First time step:
		 * Simply backup existing fields for multi-step parts of this algorithm.
		 * TODO: Maybe we should replace this with some 1st order SL time step or some subcycling
		 */
		U_phi_pert_prev = io_phi;
		U_vrt_prev = io_vrt;
		U_div_prev = io_div;
	}

	// Output variables
	SphereData_Spectral phi(sphereDataConfig);
	SphereData_Spectral vort(sphereDataConfig);
	SphereData_Spectral div(sphereDataConfig);

	/*
	 * Step 1) SL
	 * Compute Lagrangian trajectories based on SETTLS.
	 * This depends on V(t-\Delta t) and V(t).
	 *
	 * See Hortal's paper for equation.
	 */

	SphereData_Physical u_lon_prev(sphereDataConfig);
	SphereData_Physical v_lat_prev(sphereDataConfig);
	op.vortdiv_to_uv(U_vrt_prev, U_div_prev, u_lon_prev, v_lat_prev, false);

	SphereData_Physical u_lon(sphereDataConfig);
	SphereData_Physical v_lat(sphereDataConfig);
	op.vortdiv_to_uv(io_vrt, io_div, u_lon, v_lat, false);

	// Departure points and arrival points
	ScalarDataArray pos_lon_d(sphereDataConfig->physical_array_data_number_of_elements);
	ScalarDataArray pos_lat_d(sphereDataConfig->physical_array_data_number_of_elements);

	// Calculate departure points
	semiLagrangian.semi_lag_departure_points_settls(
			u_lon_prev, v_lat_prev,
			u_lon, v_lat,

			i_dt,
			i_simulation_timestamp,
			simVars.sim.sphere_radius,
			nullptr,

			pos_lon_d, pos_lat_d,		// OUTPUT

			op,

			simVars.disc.timestepping_order,
			simVars.disc.semi_lagrangian_max_iterations,
			simVars.disc.semi_lagrangian_convergence_threshold,
			simVars.disc.semi_lagrangian_approximate_sphere_geometry,
			simVars.disc.semi_lagrangian_interpolation_limiter
		);

	/*
	 * Compute phi/vort/div at departure points
	 */
	SphereData_Physical phi_D_phys(sphereDataConfig);

	sphereSampler.bicubic_scalar(
			io_phi.getSphereDataPhysical(),
			pos_lon_d, pos_lat_d,
			phi_D_phys,
			false,
			simVars.disc.semi_lagrangian_interpolation_limiter,
			simVars.disc.semi_lagrangian_sampler_use_pole_pseudo_points
		);

	SphereData_Spectral R_phi = phi_D_phys;

	SphereData_Spectral R_vrt;
	SphereData_Spectral R_div;

	/*
	 * Prepare SL Coriolis treatment
	 */
	SphereData_Physical sl_coriolis_arrival;
	SphereData_Physical sl_coriolis_departure;
	if (coriolis_treatment == CORIOLIS_SEMILAGRANGIAN)
	{
		sl_coriolis_arrival.setup(sphereDataConfig);

		/*
		 * Coriolis term
		 * 		2 \Omega r
		 * at arrival points
		 */
		sl_coriolis_arrival.physical_update_lambda(
			[&](double lon, double lat, double &io_data)
			{
				io_data = std::sin(lat)*2.0*simVars.sim.sphere_rotating_coriolis_omega*simVars.sim.sphere_radius;
			}
		);

		/*
		 * Coriolis term
		 * 		2 \Omega r
		 * at departure points
		 */
		sl_coriolis_departure = Convert_ScalarDataArray_to_SphereDataPhysical::convert(pos_lat_d, sphereDataConfig);
		sl_coriolis_departure.physical_update_lambda(
			[&](double lon, double lat, double &io_data)
			{
				io_data = std::sin(io_data)*2.0*simVars.sim.sphere_rotating_coriolis_omega*simVars.sim.sphere_radius;
			}
		);
	}


	if (coriolis_treatment != CORIOLIS_SEMILAGRANGIAN)
	{
		SphereData_Physical vort_D_phys(sphereDataConfig);
		SphereData_Physical div_D_phys(sphereDataConfig);

		sphereSampler.bicubic_scalar(
				io_vrt.getSphereDataPhysical(),
				pos_lon_d,
				pos_lat_d,
				vort_D_phys,
				false,
				simVars.disc.semi_lagrangian_interpolation_limiter,
				simVars.disc.semi_lagrangian_sampler_use_pole_pseudo_points
			);
		sphereSampler.bicubic_scalar(
				io_div.getSphereDataPhysical(),
				pos_lon_d,
				pos_lat_d,
				div_D_phys,
				false,
				simVars.disc.semi_lagrangian_interpolation_limiter,
				simVars.disc.semi_lagrangian_sampler_use_pole_pseudo_points
			);

		R_vrt = SphereData_Spectral(vort_D_phys);
		R_div = SphereData_Spectral(div_D_phys);
	}
	else
	{
		FatalError("TODO: Fix me!!!");

		SphereData_Physical u_D_phys(sphereDataConfig);
		SphereData_Physical v_D_phys(sphereDataConfig);

		/*
		 * TODO:
		 * It seems that using sl_coriolis_departure/arrival
		 * in spectral space leads to artificial high frequency modes!!!
		 */

		/*
		 * Do this in physical space, since there would be numerical oscillations if applying the sl_coriolis_arrival/depature in spectral space
		 */
		sphereSampler.bicubic_scalar(
				u_lon,
				pos_lon_d,
				pos_lat_d,
				u_D_phys,
				true,
				simVars.disc.semi_lagrangian_interpolation_limiter,
				simVars.disc.semi_lagrangian_sampler_use_pole_pseudo_points
			);
		sphereSampler.bicubic_scalar(
				v_lat,
				pos_lon_d,
				pos_lat_d,
				v_D_phys,
				true,
				simVars.disc.semi_lagrangian_interpolation_limiter,
				simVars.disc.semi_lagrangian_sampler_use_pole_pseudo_points
			);

		u_D_phys += sl_coriolis_departure;
		u_D_phys -= sl_coriolis_arrival;

		R_vrt.setup(sphereDataConfig);
		R_div.setup(sphereDataConfig);
		op.uv_to_vortdiv(
				u_D_phys, v_D_phys,
				R_vrt, R_div,
				false
			);
	}


	/*
	 * Step 2b) Solve Helmholtz problem
	 * X - 1/2 dt LX = R
	 */

	if (linear_treatment == LINEAR_IMPLICIT)
	{
		if (coriolis_treatment == CORIOLIS_LINEAR)
			swe_sphere_ts_l_irk->run_timestep_nonpert(R_phi, R_vrt, R_div, i_dt, i_simulation_timestamp);
		else
			swe_sphere_ts_lg_irk->run_timestep_nonpert(R_phi, R_vrt, R_div, i_dt, i_simulation_timestamp);
	}
	else if (linear_treatment == LINEAR_EXPONENTIAL)
	{
		swe_sphere_ts_l_rexi->run_timestep_nonpert(
				R_phi, R_vrt, R_div,
				i_dt,
				i_simulation_timestamp
		);
	}


	/*
	 * Now we care about dt*N*
	 */
	if (nonlinear_divergence_treatment == NL_DIV_NONLINEAR)
	{
		SphereData_Spectral N_phi_t(sphereDataConfig);

		/*
		 * Compute N = - Phi' * Div (U)
		 */

		/*
		 * See documentation in [sweet]/doc/swe/swe_sphere_formulation/
		 */
		/*
		 * Step SLPHI a
		 * Step SLPHI b
		 */

		// Compute N(t)
		N_phi_t.loadSphereDataPhysical((-(io_phi - gh)).getSphereDataPhysical() * io_div.getSphereDataPhysical());

		// [ 2*N(t) - N(t-dt) ]_D
		SphereData_Physical N_D_phys(sphereDataConfig);
		sphereSampler.bicubic_scalar(N_phi_t.getSphereDataPhysical(),	// field to sample
				pos_lon_d, pos_lat_d, N_D_phys, false, simVars.disc.semi_lagrangian_interpolation_limiter, simVars.disc.semi_lagrangian_sampler_use_pole_pseudo_points);

		// add nonlinear divergence
		R_phi += i_dt * N_D_phys;
	}

	if (coriolis_treatment == CORIOLIS_NONLINEAR)
	{
		/**
		 * Apply Coriolis Effect in physical VELOCITY space
		 *
		 * This is require to stay compatible to all other things
		 */

		/***************************************************************
		 * Calculate f_vort_t and f_div_t caused by Coriolis effect
		 *
		 * See /doc/swe/swe_sphere_formulation, lc_erk
		 */

		SphereData_Physical ufg = u_lon * op.fg;
		SphereData_Physical vfg = v_lat * op.fg;

		/*
		 * Step 1c
		 */
		SphereData_Spectral f_vort_t(sphereDataConfig);
		SphereData_Spectral f_div_t(sphereDataConfig);
		op.uv_to_vortdiv(ufg, vfg, f_div_t, f_vort_t, false);

		/*
		 * Step 1d
		 */
		f_vort_t *= -1.0;

		/*
		 * Extrapolation for vorticity
		 */
		// [ 2*N(t) - N(t-dt) ]_D
		SphereData_Physical f_vort_t_D_phys(sphereDataConfig);
		sphereSampler.bicubic_scalar(
				f_vort_t.getSphereDataPhysical(),	// field to sample
				pos_lon_d, pos_lat_d,
				f_vort_t_D_phys,
				false,
				simVars.disc.semi_lagrangian_interpolation_limiter,
				simVars.disc.semi_lagrangian_sampler_use_pole_pseudo_points
			);

		// add nonlinear vorticity
		R_vrt += i_dt * SphereData_Spectral(f_vort_t_D_phys);

		/**
		 * Extrapolation for divergence
		 */
		// [ 2*N(t) - N(t-dt) ]_D
		SphereData_Physical f_div_t_D_phys(sphereDataConfig);
		sphereSampler.bicubic_scalar(
				f_div_t.getSphereDataPhysical(),	// field to sample
				pos_lon_d, pos_lat_d,
				f_div_t_D_phys,
				false,
				simVars.disc.semi_lagrangian_interpolation_limiter,
				simVars.disc.semi_lagrangian_sampler_use_pole_pseudo_points
			);

		// add nonlinear divergence
		R_div += i_dt * SphereData_Spectral(f_div_t_D_phys);
	}

	io_phi = R_phi;
	io_vrt = R_vrt;
	io_div = R_div;
}



void SWE_Sphere_TS_ln_settls::run_timestep_2nd_order(
	SphereData_Spectral &io_U_phi,	///< prognostic variables
	SphereData_Spectral &io_U_vrt,	///< prognostic variables
	SphereData_Spectral &io_U_div,	///< prognostic variables

	double i_dt,					///< if this value is not equal to 0, use this time step size instead of computing one
	double i_simulation_timestamp
)
{

	const SphereData_Config *sphereDataConfig = io_U_phi.sphereDataConfig;
	double gh0 = simVars.sim.gravitation * simVars.sim.h0;
//	double dt_radius = i_dt/simVars.sim.sphere_radius;

	if (i_dt <= 0)
		FatalError("SWE_Sphere_TS_ln_settls: Only constant time step size allowed (Please set --dt)");

	if (i_simulation_timestamp == 0)
	{
		/*
		 * First time step:
		 * Simply backup existing fields for multi-step parts of this algorithm.
		 * TODO: Maybe we should replace this with some 1st order SL time step or some subcycling
		 */
		U_phi_pert_prev = io_U_phi;
		U_vrt_prev = io_U_vrt;
		U_div_prev = io_U_div;
	}


	/*
	 * Step 1) SL
	 * Compute Lagrangian trajectories based on SETTLS.
	 * This depends on V(t-\Delta t) and V(t).
	 *
	 * See Hortal's paper for equation.
	 */

	SphereData_Physical u_lon_prev(sphereDataConfig);
	SphereData_Physical v_lat_prev(sphereDataConfig);
	op.vortdiv_to_uv(U_vrt_prev, U_div_prev, u_lon_prev, v_lat_prev, false);

	SphereData_Physical U_u(sphereDataConfig);
	SphereData_Physical U_v(sphereDataConfig);
	op.vortdiv_to_uv(io_U_vrt, io_U_div, U_u, U_v, false);

	// Departure points and arrival points
	ScalarDataArray pos_lon_d(sphereDataConfig->physical_array_data_number_of_elements);
	ScalarDataArray pos_lat_d(sphereDataConfig->physical_array_data_number_of_elements);

	// Calculate departure points
	semiLagrangian.semi_lag_departure_points_settls(
			u_lon_prev, v_lat_prev,
			U_u, U_v,

			i_dt,
			i_simulation_timestamp,
			simVars.sim.sphere_radius,
			nullptr,

			pos_lon_d, pos_lat_d,		// OUTPUT

			op,

			simVars.disc.timestepping_order,
			simVars.disc.semi_lagrangian_max_iterations,
			simVars.disc.semi_lagrangian_convergence_threshold,
			simVars.disc.semi_lagrangian_approximate_sphere_geometry,
			simVars.disc.semi_lagrangian_interpolation_limiter
	);

	/*
	 * Step 2) Midpoint rule
	 * Put everything together with midpoint rule and solve resulting Helmholtz problem
	 */

	/*
	 * Step 2a) Compute RHS
	 * R = X_D + 1/2 dt L_D + dt N*
	 */

	/*
	 * Compute X_D
	 */
	SphereData_Spectral U_phi_D;
	SphereData_Spectral U_vrt_D;
	SphereData_Spectral U_div_D;

	if (coriolis_treatment != CORIOLIS_SEMILAGRANGIAN)
	{
		if (nonlinear_advection_treatment == NL_ADV_SEMILAGRANGIAN)
		{
			SphereData_Physical U_phi_D_phys(sphereDataConfig);
			sphereSampler.bicubic_scalar(
					io_U_phi.getSphereDataPhysical(),
					pos_lon_d, pos_lat_d,
					U_phi_D_phys,
					false,
					simVars.disc.semi_lagrangian_interpolation_limiter,
					simVars.disc.semi_lagrangian_sampler_use_pole_pseudo_points
				);
			U_phi_D = U_phi_D_phys;

#if 0

			SphereData_Physical U_vort_D_phys(sphereDataConfig);
			sphereSampler.bicubic_scalar(io_U_vrt.getSphereDataPhysical(), pos_lon_d, pos_lat_d, U_vort_D_phys, false, simVars.disc.semi_lagrangian_interpolation_limiter);
			U_vrt_D = U_vort_D_phys;

			SphereData_Physical U_div_D_phys(sphereDataConfig);
			sphereSampler.bicubic_scalar(io_U_div.getSphereDataPhysical(), pos_lon_d, pos_lat_d, U_div_D_phys, false, simVars.disc.semi_lagrangian_interpolation_limiter);
			U_div_D = U_div_D_phys;

#else

			SphereData_Physical U_u_D_phys(sphereDataConfig);
			sphereSampler.bicubic_scalar(U_u, pos_lon_d, pos_lat_d, U_u_D_phys, true, simVars.disc.semi_lagrangian_interpolation_limiter, simVars.disc.semi_lagrangian_sampler_use_pole_pseudo_points);

			SphereData_Physical U_v_D_phys(sphereDataConfig);
			sphereSampler.bicubic_scalar(U_v, pos_lon_d, pos_lat_d, U_v_D_phys, true, simVars.disc.semi_lagrangian_interpolation_limiter, simVars.disc.semi_lagrangian_sampler_use_pole_pseudo_points);

			U_vrt_D.setup(sphereDataConfig);
			U_div_D.setup(sphereDataConfig);
			op.uv_to_vortdiv(U_u_D_phys, U_v_D_phys, U_vrt_D, U_div_D, false);

#endif

		}
		else
		{
			U_phi_D = io_U_phi;
			U_vrt_D = io_U_vrt;
			U_div_D = io_U_div;
		}
	}
	else
	{
		if (nonlinear_advection_treatment == NL_ADV_SEMILAGRANGIAN)
			FatalError("Only Coriolis semi-lagrangian advection is not supported");

		FatalError("TODO: Fix me!!!");

		SphereData_Spectral U_vort_D(sphereDataConfig);
		SphereData_Spectral U_div_D(sphereDataConfig);

		/*
		 * Prepare SL Coriolis treatment
		 */
		SphereData_Physical sl_coriolis_arrival;
		SphereData_Physical sl_coriolis_departure;
		if (coriolis_treatment == CORIOLIS_SEMILAGRANGIAN)
		{
			sl_coriolis_arrival.setup(sphereDataConfig);

			/*
			 * Coriolis term
			 * 		2 \Omega r
			 * at arrival points
			 */
			sl_coriolis_arrival.physical_update_lambda(
				[&](double lon, double lat, double &io_data)
				{
					io_data = std::sin(lat)*2.0*simVars.sim.sphere_rotating_coriolis_omega*simVars.sim.sphere_radius;
				}
			);

			/*
			 * Coriolis term
			 * 		2 \Omega r
			 * at departure points
			 */
			sl_coriolis_departure = Convert_ScalarDataArray_to_SphereDataPhysical::convert(pos_lat_d, sphereDataConfig);
			sl_coriolis_departure.physical_update_lambda(
				[&](double lon, double lat, double &io_data)
				{
					io_data = std::sin(io_data)*2.0*simVars.sim.sphere_rotating_coriolis_omega*simVars.sim.sphere_radius;
				}
			);
		}


		if (coriolis_treatment != CORIOLIS_SEMILAGRANGIAN)
		{
			FatalError("TODO: Make sure this is correct!");

			SphereData_Physical U_vort_D_phys(sphereDataConfig);
			SphereData_Physical U_div_D_phys(sphereDataConfig);

			sphereSampler.bicubic_scalar(
					io_U_vrt.getSphereDataPhysical(),
					pos_lon_d, pos_lat_d,
					U_vort_D_phys,
					false,
					simVars.disc.semi_lagrangian_interpolation_limiter,
					simVars.disc.semi_lagrangian_sampler_use_pole_pseudo_points
				);
			sphereSampler.bicubic_scalar(
					io_U_div.getSphereDataPhysical(),
					pos_lon_d,
					pos_lat_d, U_div_D_phys,
					false,
					simVars.disc.semi_lagrangian_interpolation_limiter,
					simVars.disc.semi_lagrangian_sampler_use_pole_pseudo_points
				);

			U_vort_D.loadSphereDataPhysical(U_vort_D_phys);
			U_div_D.loadSphereDataPhysical(U_div_D_phys);
		}
		else
		{
			/*
			 * TODO:
			 * It seems that using sl_coriolis_departure/arrival
			 * in spectral space leads to artificial high frequency modes!!!
			 */

			FatalError("TODO: Get this running!");

			SphereData_Physical U_u_D_phys(sphereDataConfig);
			SphereData_Physical U_v_D_phys(sphereDataConfig);

			/*
			 * Do this in physical space, since there would be numerical oscillations if applying the sl_coriolis_arrival/depature in spectral space
			 */
			sphereSampler.bicubic_scalar(
					U_u,
					pos_lon_d,
					pos_lat_d,
					U_u_D_phys,
					true,
					simVars.disc.semi_lagrangian_interpolation_limiter,
					simVars.disc.semi_lagrangian_sampler_use_pole_pseudo_points
				);
			sphereSampler.bicubic_scalar(
					U_v,
					pos_lon_d,
					pos_lat_d,
					U_v_D_phys,
					true,
					simVars.disc.semi_lagrangian_interpolation_limiter,
					simVars.disc.semi_lagrangian_sampler_use_pole_pseudo_points
				);

			U_u_D_phys += sl_coriolis_departure;
			U_u_D_phys -= sl_coriolis_arrival;

			op.uv_to_vortdiv(
					U_u_D_phys, U_v_D_phys,
					U_vort_D, U_div_D,
					false
				);
		}
	}

	/*
	 * Compute L_D
	 */

	SphereData_Spectral L_phi_D(sphereDataConfig);
	SphereData_Spectral L_vort_D(sphereDataConfig);
	SphereData_Spectral L_div_D(sphereDataConfig);

	if (original_linear_operator_sl_treatment)
	{
		/*
		 * Method 1) First evaluate L, then sample result at departure point
		 */
		SphereData_Spectral L_phi(sphereDataConfig);
		SphereData_Spectral L_vort(sphereDataConfig);
		SphereData_Spectral L_div(sphereDataConfig);

		if (coriolis_treatment == CORIOLIS_LINEAR)
		{
			swe_sphere_ts_l_erk->euler_timestep_update(io_U_phi, io_U_vrt, io_U_div, L_phi, L_vort, L_div, i_simulation_timestamp);
		}
		else
		{
			swe_sphere_ts_lg_erk->euler_timestep_update(io_U_phi, io_U_vrt, io_U_div, L_phi, L_vort, L_div, i_simulation_timestamp);
		}

		SphereData_Physical L_phi_D_phys(sphereDataConfig);
		sphereSampler.bicubic_scalar(
				L_phi.getSphereDataPhysical(),
				pos_lon_d, pos_lat_d, L_phi_D_phys,
				false,
				simVars.disc.semi_lagrangian_interpolation_limiter,
				simVars.disc.semi_lagrangian_sampler_use_pole_pseudo_points
			);
		L_phi_D.loadSphereDataPhysical(L_phi_D_phys);

#if 0

		SphereData_Physical L_vort_D_phys(sphereDataConfig);
		sphereSampler.bicubic_scalar(L_vort.getSphereDataPhysical(), pos_lon_d, pos_lat_d, L_vort_D_phys, false, simVars.disc.semi_lagrangian_interpolation_limiter);
		L_vort_D.loadSphereDataPhysical(L_vort_D_phys);

		SphereData_Physical L_div_D_phys(sphereDataConfig);
		sphereSampler.bicubic_scalar(L_div.getSphereDataPhysical(), pos_lon_d, pos_lat_d, L_div_D_phys, false, simVars.disc.semi_lagrangian_interpolation_limiter);
		L_div_D.loadSphereDataPhysical(L_div_D_phys);

#else

		SphereData_Physical U_L_u_phys(sphereDataConfig);
		SphereData_Physical U_L_v_phys(sphereDataConfig);
		op.vortdiv_to_uv(L_vort, L_div, U_L_u_phys, U_L_v_phys, false);

		SphereData_Physical U_u_D_phys(sphereDataConfig);
		sphereSampler.bicubic_scalar(
				U_L_u_phys,
				pos_lon_d, pos_lat_d, U_u_D_phys,
				true,
				simVars.disc.semi_lagrangian_interpolation_limiter,
				simVars.disc.semi_lagrangian_sampler_use_pole_pseudo_points
			);

		SphereData_Physical U_v_D_phys(sphereDataConfig);
		sphereSampler.bicubic_scalar(
				U_L_v_phys,
				pos_lon_d, pos_lat_d,
				U_v_D_phys,
				true,
				simVars.disc.semi_lagrangian_interpolation_limiter,
				simVars.disc.semi_lagrangian_sampler_use_pole_pseudo_points
			);

		op.uv_to_vortdiv(U_u_D_phys, U_v_D_phys, L_vort_D, L_div_D, false);

#endif
	}
	else
	{
		/*
		 * Method 2) First get variables on departure points, then evaluate L
		 */

		SphereData_Physical U_phi_D_phys(sphereDataConfig);
		sphereSampler.bicubic_scalar(
				io_U_phi.getSphereDataPhysical(),
				pos_lon_d, pos_lat_d,
				U_phi_D_phys,
				false,
				simVars.disc.semi_lagrangian_interpolation_limiter,
				simVars.disc.semi_lagrangian_sampler_use_pole_pseudo_points
			);
		SphereData_Spectral U_phi_D(sphereDataConfig);
		U_phi_D.loadSphereDataPhysical(U_phi_D_phys);

		SphereData_Physical U_vort_D_phys(sphereDataConfig);
		SphereData_Physical U_div_D_phys(sphereDataConfig);

		FatalError("Get this running with U/V");

		sphereSampler.bicubic_scalar(
				io_U_vrt.getSphereDataPhysical(),
				pos_lon_d, pos_lat_d,
				U_vort_D_phys,
				false,
				simVars.disc.semi_lagrangian_interpolation_limiter,
				simVars.disc.semi_lagrangian_sampler_use_pole_pseudo_points
			);
		sphereSampler.bicubic_scalar(
				io_U_div.getSphereDataPhysical(),
				pos_lon_d, pos_lat_d,
				U_div_D_phys,
				false,
				simVars.disc.semi_lagrangian_interpolation_limiter,
				simVars.disc.semi_lagrangian_sampler_use_pole_pseudo_points
			);

		SphereData_Spectral U_vrt_D(sphereDataConfig);
		SphereData_Spectral U_div_D(sphereDataConfig);

		U_vrt_D.loadSphereDataPhysical(U_vort_D_phys);
		U_div_D.loadSphereDataPhysical(U_div_D_phys);

		if (coriolis_treatment == CORIOLIS_LINEAR)
			swe_sphere_ts_l_erk->euler_timestep_update(U_phi_D, U_vrt_D, U_div_D, L_phi_D, L_vort_D, L_div_D, i_simulation_timestamp);
		else
			swe_sphere_ts_lg_erk->euler_timestep_update(U_phi_D, U_vrt_D, U_div_D, L_phi_D, L_vort_D, L_div_D, i_simulation_timestamp);
	}

	/*
	 * Compute R = X_D + 1/2 dt L_D + dt N*
	 */

	SphereData_Spectral R_phi(sphereDataConfig);
	SphereData_Spectral R_vrt(sphereDataConfig);
	SphereData_Spectral R_div(sphereDataConfig);

	R_phi = U_phi_D + 0.5 * i_dt * L_phi_D;
	R_vrt = U_vrt_D + 0.5 * i_dt * L_vort_D;
	R_div = U_div_D + 0.5 * i_dt * L_div_D;

	/*
	 * Now we care about dt*N*
	 */
	if (nonlinear_divergence_treatment == NL_DIV_NONLINEAR)
	{
		/**
		 * Compute non-linear term N*
		 *
		 * Extrapolate non-linear terms with the equation
		 * N*(t+0.5dt) = 1/2 [ 2*N(t) - N(t-dt) ]_D + N^t
		 *
		 * Here we assume that
		 * 		N = -Phi' * Div (U)
		 * with non-constant U!
		 * 
		 * The divergence is already 
		 */

		SphereData_Spectral N_phi_t(sphereDataConfig);
		SphereData_Spectral N_phi_prev(sphereDataConfig);

		/*
		 * Compute N = - Phi' * Div (U)
		 */

		/*
		 * See documentation in [sweet]/doc/swe/swe_sphere_formulation/
		 */
		/*
		 * Step SLPHI a
		 * Step SLPHI b
		 */

		// Compute N(t)
		N_phi_t.loadSphereDataPhysical((-(io_U_phi - gh0)).getSphereDataPhysical() * io_U_div.getSphereDataPhysical());

		// Compute N(t-dt)
		N_phi_prev.loadSphereDataPhysical((-(U_phi_pert_prev - gh0)).getSphereDataPhysical() * U_div_prev.getSphereDataPhysical());

		// [ 2*N(t) - N(t-dt) ]_D
		SphereData_Physical N_D_phys(sphereDataConfig);
		sphereSampler.bicubic_scalar(
				(2.0 * N_phi_t - N_phi_prev).getSphereDataPhysical(),	// field to sample
				pos_lon_d, pos_lat_d,
				N_D_phys,
				false,
				simVars.disc.semi_lagrangian_interpolation_limiter,
				simVars.disc.semi_lagrangian_sampler_use_pole_pseudo_points
			);

		// N_D spectral
		SphereData_Spectral N_D(N_D_phys);

		// Compute N*(t+0.5dt) = 1/2 ([ 2*N(t) - N(t-dt) ]_D + N^t)
		SphereData_Spectral N_star = 0.5 * (N_D + N_phi_t);

		// add nonlinear divergence
		R_phi += i_dt * N_star;
	}

	if (coriolis_treatment == CORIOLIS_NONLINEAR)
	{
		/**
		 * Apply Coriolis Effect in physical VELOCITY space
		 * 
		 * This is require to stay compatible to all other things
		 */

		/***************************************************************
		 * Calculate f_vort_t and f_div_t caused by Coriolis effect
		 *
		 * See /doc/swe/swe_sphere_formulation, lc_erk
		 */
		/*
		 * Step 1a
		 */
		// u_lon and u_lat already available in physical space
		/*
		 * Step 1b
		 */
		SphereData_Physical ufg = U_u * op.fg;
		SphereData_Physical vfg = U_v * op.fg;

		/*
		 * Step 1c
		 */
		SphereData_Spectral f_vort_t(sphereDataConfig);
		SphereData_Spectral f_div_t(sphereDataConfig);
		op.uv_to_vortdiv(ufg, vfg, f_div_t, f_vort_t, false);

		/*
		 * Step 1d
		 */
		f_vort_t *= -1.0;

		/*
		 * Step 1e
		 */
		// f_div_t already assigned

		/***************************************************************
		 * Calculate f_vort_t_prev and f_div_t_prev caused by Coriolis effect
		 *
		 * See /doc/swe/swe_sphere_formulation, lc_erk
		 */

		/*
		 * Step 1b
		 */

		// u_lon and u_lat already available in physical space
		SphereData_Physical ufg_prev = u_lon_prev * op.fg;
		SphereData_Physical vfg_prev = v_lat_prev * op.fg;

		/*
		 * Step 1c
		 */
		SphereData_Spectral f_vort_t_prev(sphereDataConfig);
		SphereData_Spectral f_div_t_prev(sphereDataConfig);
		op.uv_to_vortdiv(ufg_prev, vfg_prev, f_div_t_prev, f_vort_t_prev, false);


		/*
		 * Step 1d
		 */
		f_vort_t_prev *= -1.0;

		/*
		 * Step 1e
		 */
		// f_div_t_prev already assigned

		/*******************************************************************
		 * Extrapolation of Coriolis effect
		 */

#if 0

		/*
		 * Extrapolation for vorticity
		 */
		// [ 2*N(t) - N(t-dt) ]_D
		SphereData_Physical f_vort_t_D_phys(sphereDataConfig);
		sphereSampler.bicubic_scalar(
				(2.0 * f_vort_t - f_vort_t_prev).getSphereDataPhysical(),	// field to sample
				pos_lon_d, pos_lat_d,
				f_vort_t_D_phys,
				false,
				simVars.disc.semi_lagrangian_interpolation_limiter
			);

		/*
		 * Extrapolation for divergence
		 */
		// [ 2*N(t) - N(t-dt) ]_D
		SphereData_Physical f_div_t_D_phys(sphereDataConfig);
		sphereSampler.bicubic_scalar(
				(2.0 * f_div_t - f_div_t_prev).getSphereDataPhysical(),	// field to sample
				pos_lon_d, pos_lat_d,
				f_div_t_D_phys,
				false,
				simVars.disc.semi_lagrangian_interpolation_limiter
			);

		// vort_D physical
		SphereData_Spectral f_vrt_t_D(f_vort_t_D_phys);

		// div_D physical
		SphereData_Spectral f_div_t_D(f_div_t_D_phys);

#else

		SphereData_Physical f_u_t(sphereDataConfig);
		SphereData_Physical f_v_t(sphereDataConfig);

		op.vortdiv_to_uv(
				2.0 * f_vort_t - f_vort_t_prev,
				2.0 * f_div_t - f_div_t_prev,
				f_u_t,
				f_v_t,
				false
		);

		SphereData_Physical f_u_t_D(sphereDataConfig);
		SphereData_Physical f_v_t_D(sphereDataConfig);

		sphereSampler.bicubic_scalar(
				f_u_t,
				pos_lon_d, pos_lat_d,
				f_u_t_D,
				false,
				simVars.disc.semi_lagrangian_interpolation_limiter,
				simVars.disc.semi_lagrangian_sampler_use_pole_pseudo_points
			);

		sphereSampler.bicubic_scalar(
				f_v_t,
				pos_lon_d, pos_lat_d,
				f_v_t_D,
				false,
				simVars.disc.semi_lagrangian_interpolation_limiter,
				simVars.disc.semi_lagrangian_sampler_use_pole_pseudo_points
			);


		SphereData_Spectral f_vrt_t_D(sphereDataConfig);
		SphereData_Spectral f_div_t_D(sphereDataConfig);
		op.uv_to_vortdiv(
				f_u_t_D,
				f_v_t_D,
				f_vrt_t_D,
				f_div_t_D,
				false
		);
#endif

		// Compute N*(t+0.5dt) = 1/2 ([ 2*N(t) - N(t-dt) ]_D + N^t)
		SphereData_Spectral f_vort_t_star = 0.5 * (f_vrt_t_D + f_vort_t);

		// add nonlinear vorticity
		R_vrt += i_dt * f_vort_t_star;


		// Compute N*(t+0.5dt) = 1/2 ([ 2*N(t) - N(t-dt) ]_D + N^t)
		SphereData_Spectral f_div_t_star = 0.5 * (f_div_t_D + f_div_t);

		// add nonlinear divergence
		R_div += i_dt * f_div_t_star;
	}

	/*
	 * Step 2b) Solve Helmholtz problem
	 * X - 1/2 dt LX = R
	 */
	if (linear_treatment == LINEAR_IMPLICIT)
	{
		if (coriolis_treatment == CORIOLIS_LINEAR)
		{
			swe_sphere_ts_l_irk->run_timestep_nonpert(
					R_phi, R_vrt, R_div,
					0.5 * i_dt,
					i_simulation_timestamp
				);
		}
		else
		{
			swe_sphere_ts_lg_irk->run_timestep_nonpert(
					R_phi, R_vrt, R_div,
					0.5 * i_dt,
					i_simulation_timestamp
				);
		}
	}
	else if (linear_treatment == LINEAR_EXPONENTIAL)
	{
		swe_sphere_ts_l_rexi->run_timestep_nonpert(
			R_phi, R_vrt, R_div,
			0.5 * i_dt,
			i_simulation_timestamp
		);
	}

	/*
	 * Backup previous variables for multi-step SL method
	 */
	U_phi_pert_prev = io_U_phi;
	U_vrt_prev = io_U_vrt;
	U_div_prev = io_U_div;

	/*
	 * Return new fields stored in R_*
	 */
	io_U_phi = R_phi;
	io_U_vrt = R_vrt;
	io_U_div = R_div;
}



void SWE_Sphere_TS_ln_settls::setup(
		int i_timestepping_order,
		LinearTreatment_enum i_linear_treatment,
		LinearCoriolisTreatment_enum i_coriolis_treatment,
		NLAdvectionTreatment_enum i_nonlinear_advection_treatment,
		NLDivergenceTreatment_enum i_nonlinear_divergence_treatment,
		bool i_original_linear_operator_sl_treatment
)
{
	std::cout << " + SWE_Sphere_TS_ln_settls.setup() called" << std::endl;

	linear_treatment = i_linear_treatment;
	coriolis_treatment = i_coriolis_treatment;
	nonlinear_advection_treatment = i_nonlinear_advection_treatment;
	nonlinear_divergence_treatment = i_nonlinear_divergence_treatment;
	timestepping_order = i_timestepping_order;
	original_linear_operator_sl_treatment = i_original_linear_operator_sl_treatment;

	if (nonlinear_advection_treatment == NL_ADV_IGNORE)
	{
		std::cerr << "Using no treatment of Semi-Lagrangian advection would result in instabilities" << std::endl;
		std::cerr << "Benchmark: geostropic_balance, timestepping method: l_irk_settls, happens after about 3 days with checkerboard pattern" << std::endl;
		FatalError("Doesn't make much sense");
	}

	if (timestepping_order != 1 && timestepping_order != 2)
		FatalError("Invalid time stepping order, must be 1 or 2");

	// Setup sampler for future interpolations
	sphereSampler.setup(op.sphereDataConfig);

	// Setup semi-lag
	semiLagrangian.setup(op.sphereDataConfig, simVars);


	swe_sphere_ts_l_erk = nullptr;
	swe_sphere_ts_l_irk = nullptr;
	swe_sphere_ts_lg_erk = nullptr;
	swe_sphere_ts_lg_irk = nullptr;
	swe_sphere_ts_l_rexi = nullptr;


	if (linear_treatment == LINEAR_EXPONENTIAL)
	{
		swe_sphere_ts_l_rexi = new SWE_Sphere_TS_l_rexi(simVars, op);

		if (timestepping_order == 1)
			swe_sphere_ts_l_rexi->setup(
					simVars.rexi,
					"phi0",
					simVars.timecontrol.current_timestep_size,
					simVars.sim.sphere_use_fsphere,
					coriolis_treatment != CORIOLIS_LINEAR
				);
		else
			swe_sphere_ts_l_rexi->setup(
					simVars.rexi,
					"phi0",
					0.5 * simVars.timecontrol.current_timestep_size,
					simVars.sim.sphere_use_fsphere,
					coriolis_treatment != CORIOLIS_LINEAR
				);

	}

	if (coriolis_treatment == CORIOLIS_LINEAR)
	{
		// initialize with 1st order
		swe_sphere_ts_l_erk = new SWE_Sphere_TS_l_erk(simVars, op);
		swe_sphere_ts_l_erk->setup(1);

		if (timestepping_order == 1)
		{
			if (linear_treatment == LINEAR_IMPLICIT)
			{
				swe_sphere_ts_l_irk = new SWE_Sphere_TS_l_irk(simVars, op);
				swe_sphere_ts_l_irk->setup(1, simVars.timecontrol.current_timestep_size, 0);
			}
		}
		else
		{
			if (linear_treatment == LINEAR_IMPLICIT)
			{
				// initialize with 1st order and half time step size
				swe_sphere_ts_l_irk = new SWE_Sphere_TS_l_irk(simVars, op);
				swe_sphere_ts_l_irk->setup(1, 0.5 * simVars.timecontrol.current_timestep_size, 0);
			}
		}
	}
	else
	{
		// initialize with 1st order
		swe_sphere_ts_lg_erk = new SWE_Sphere_TS_lg_erk(simVars, op);
		swe_sphere_ts_lg_erk->setup(1);

		swe_sphere_ts_lg_irk = new SWE_Sphere_TS_lg_irk(simVars, op);
		if (timestepping_order == 1)
			swe_sphere_ts_lg_irk->setup(1, simVars.timecontrol.current_timestep_size, 0);
		else
			// initialize with 1st order and half time step size
			swe_sphere_ts_lg_irk->setup(1, 0.5 * simVars.timecontrol.current_timestep_size, 0);
	}
}

SWE_Sphere_TS_ln_settls::SWE_Sphere_TS_ln_settls(
			SimulationVariables &i_simVars,
			SphereOperators_SphereData &i_op
		) :
		simVars(i_simVars), op(i_op),

		U_phi_pert_prev(i_op.sphereDataConfig),
		U_vrt_prev(i_op.sphereDataConfig),
		U_div_prev(i_op.sphereDataConfig)
{
}

SWE_Sphere_TS_ln_settls::~SWE_Sphere_TS_ln_settls()
{
	delete swe_sphere_ts_l_erk;
	delete swe_sphere_ts_lg_erk;
	delete swe_sphere_ts_l_irk;
	delete swe_sphere_ts_lg_irk;

	delete swe_sphere_ts_l_rexi;
}

