/*
 * Burgers_Plane_TS_ln_imex.hpp
 *
 *  Created on: 17 June 2017
 *  Author: Andreas Schmitt <aschmitt@fnb.tu-darmstadt.de>
 */

#ifndef SRC_PROGRAMS_BURGERS_PLANE_TS_LN_IMEX_HPP_
#define SRC_PROGRAMS_BURGERS_PLANE_TS_LN_IMEX_HPP_

#include <limits>
#include <sweet/plane/PlaneData_Spectral.hpp>
#include <sweet/SimulationVariables.hpp>
#include <sweet/plane/PlaneOperators.hpp>
#include <cmath>
#include <sweet/plane/PlaneDataTimesteppingExplicitRK.hpp>
#include <sweet/plane/PlaneStaggering.hpp>
#include <sweet/SWEETError.hpp>
#include "../burgers_timeintegrators/Burgers_Plane_TS_interface.hpp"

class Burgers_Plane_TS_ln_imex	: public Burgers_Plane_TS_interface
{
	SimulationVariables &simVars;
	PlaneOperators &op;

	int timestepping_order;
	PlaneDataTimesteppingExplicitRK timestepping_rk;

public:
	Burgers_Plane_TS_ln_imex(
			SimulationVariables &i_simVars,
			PlaneOperators &i_op
		);

	void setup(
			int i_order	///< order of RK time stepping method
	);

	void run_timestep(
			PlaneData_Spectral &io_u,	///< prognostic variables
			PlaneData_Spectral &io_v,	///< prognostic variables
			///PlaneData_Spectral &io_u_prev,	///< prognostic variables
			///PlaneData_Spectral &io_v_prev,	///< prognostic variables

			double i_fixed_dt = 0,
			double i_simulation_timestamp = -1
	);



	virtual ~Burgers_Plane_TS_ln_imex();
};

#endif /* SRC_PROGRAMS_BURGERS_PLANE_TS_LN_IMEX_HPP_ */
