/*
 * SWE_Sphere_TS_l_na_settls_only.hpp
 *
 *  Created on: 24 Sep 2019
 *      Author: Martin Schreiber <SchreiberX@gmail.com>
 *
 *  Based on plane code
 */

#ifndef SRC_PROGRAMS_SWE_SPHERE_REXI_SWE_SPHERE_TS_L_IRK_NA_SL_SETTLS_ONLY_HPP_
#define SRC_PROGRAMS_SWE_SPHERE_REXI_SWE_SPHERE_TS_L_IRK_NA_SL_SETTLS_ONLY_HPP_

#include <limits>
#include <sweet/SimulationVariables.hpp>
#include <sweet/sphere/SphereData_Physical.hpp>
#include <sweet/sphere/SphereData_Spectral.hpp>
#include <sweet/sphere/SphereOperators_SphereData.hpp>
#include <sweet/sphere/SphereOperators_Sampler_SphereDataPhysical.hpp>
#include <sweet/sphere/SphereTimestepping_SemiLagrangian.hpp>
#include <sweet/sphere/SphereTimestepping_ExplicitRK.hpp>

#include "SWE_Sphere_TS_interface.hpp"
#include "SWE_Sphere_TS_l_irk.hpp"
#include "SWE_Sphere_TS_lg_irk.hpp"
#include "SWE_Sphere_TS_l_erk.hpp"
#include "SWE_Sphere_TS_lg_erk.hpp"
#include "SWE_Sphere_TS_l_rexi.hpp"



class SWE_Sphere_TS_l_irk_na_sl_settls_only	: public SWE_Sphere_TS_interface
{
	SimulationVariables &simVars;
	SphereOperators_SphereData &op;

private:
	int timestepping_order;

	SphereTimestepping_SemiLagrangian semiLagrangian;
	SphereOperators_Sampler_SphereDataPhysical sphereSampler;

	SphereData_Spectral U_phi_pert_prev, U_vrt_prev, U_div_prev;

	SWE_Sphere_TS_l_erk* swe_sphere_ts_l_erk;
	SWE_Sphere_TS_l_irk* swe_sphere_ts_l_irk;



public:
	SWE_Sphere_TS_l_irk_na_sl_settls_only(
			SimulationVariables &i_simVars,
			SphereOperators_SphereData &i_op
		);


	void setup(
			int i_timestepping_order
	);


	void run_timestep_pert(
			SphereData_Spectral &io_phi,	///< prognostic variables
			SphereData_Spectral &io_vort,	///< prognostic variables
			SphereData_Spectral &io_div,	///< prognostic variables

			double i_dt = 0,		///< if this value is not equal to 0, use this time step size instead of computing one
			double i_simulation_timestamp = -1
	);

	void run_timestep_2nd_order_pert(
			SphereData_Spectral &io_phi_pert,	///< prognostic variables
			SphereData_Spectral &io_vort,		///< prognostic variables
			SphereData_Spectral &io_div,		///< prognostic variables

			double i_dt = 0,		///< if this value is not equal to 0, use this time step size instead of computing one
			double i_simulation_timestamp = -1
	);

	virtual ~SWE_Sphere_TS_l_irk_na_sl_settls_only();
};

#endif /* SRC_PROGRAMS_SWE_SPHERE_REXI_SWE_SPHERE_TS_L_CN_NA_SL_ND_SETTLS_HPP_ */
