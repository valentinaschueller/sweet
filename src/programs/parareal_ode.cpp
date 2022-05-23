/*
 * Author: Martin Schreiber <SchreiberX@gmail.com>
 */

//////////#if !SWEET_PARAREAL
//////////#	error "Parareal not activated"
//////////#endif

#include <limits>
#include <stdlib.h>

#if SWEET_PARAREAL
#include <parareal/Parareal.hpp>
#include <parareal/Parareal_GenericData.hpp>
#include <parareal/Parareal_GenericData_Scalar.hpp>
#include <parareal/Parareal_Controller_Serial_GenericData.hpp>
#endif

#include <cmath>
#include <sweet/SimulationVariables.hpp>


SimulationVariables simVars;

double param_parareal_fine_dt = -1;
double param_parareal_function_y0 = 0.123;
double param_parareal_function_a = 0.1;
double param_parareal_function_b = 1.0;

double param_fine_timestepping_solution = std::numeric_limits<double>::infinity();

/*
 * ODE implementation of the Parareal algorithm to test implementations.
 *
 * Usage of the program:
 * --parareal-fine-dt=0.0001 --parareal-enabled=1 --parareal-coarse-slices=10 -t 10 --parareal-convergence-threshold=0.0001 --parareal-function-param-a=0.3 --parareal-function-param-b=1.0 --parareal-function-param-y0=0.123
 *
 */

class ODE_Scalar_TS_interface
{
private:
	double u_prev;

public:
	void run_timestep(
			double &io_y,			///< prognostic variables

			double i_dt,		///< time step size
			double i_sim_timestamp
	)
	{
		double a = param_parareal_function_a;
		double b = param_parareal_function_b;

		io_y += i_dt * (a * std::sin(io_y) + b * std::sin(i_sim_timestamp));
	}

#if SWEET_PARAREAL
	void run_timestep(
			Parareal_GenericData* io_data,

			double i_dt,		///< time step size
			double i_sim_timestamp
	)
	{
		double y = io_data->get_pointer_to_data_Scalar()->simfields[0];

		run_timestep(y,
				i_dt,
				i_sim_timestamp
			);

		io_data->get_pointer_to_data_Scalar()->simfields[0] = y;

	}

	// for parareal SL (not needed here)
	void set_previous_solution(
			Parareal_GenericData* i_data
	)
	{
		u_prev = i_data->get_pointer_to_data_Scalar()->simfields[0];
	};
#endif
};




class ODE_Scalar_TimeSteppers
{
public:
	ODE_Scalar_TS_interface *master = nullptr;

	ODE_Scalar_TimeSteppers()
	{
	}

	void reset()
	{
		if (master != nullptr)
		{
			delete master;
			master = nullptr;
		}
	}

	void setup(
			const std::string &i_timestepping_method,
			int &i_timestepping_order,

			SimulationVariables &i_simVars
	)
	{
		reset();
		master = new ODE_Scalar_TS_interface;
	}

	~ODE_Scalar_TimeSteppers()
	{
		reset();
	}
};



class SimulationInstance
{


private:
	double prog_u;
	SimulationVariables* simVars;

public:
	ODE_Scalar_TimeSteppers* timeSteppers = nullptr;

public:
	SimulationInstance(SimulationVariables* i_simVars)
		: simVars(i_simVars)
	{
		timeSteppers = new ODE_Scalar_TimeSteppers;
	}

	~SimulationInstance()
	{
		if (timeSteppers)
		{
			delete timeSteppers;
			timeSteppers = nullptr;
		}
	}

public:
	void run()
	{
		// reset simulation time
		simVars->timecontrol.current_simulation_time = 0;
		simVars->timecontrol.current_timestep_nr = 0;
		this->prog_u = param_parareal_function_y0;

		while (true)
		{
			this->timeSteppers->master->run_timestep(this->prog_u,
					simVars->timecontrol.current_timestep_size,
					simVars->timecontrol.current_simulation_time
				);

			simVars->timecontrol.current_simulation_time += simVars->timecontrol.current_timestep_size;
			simVars->timecontrol.current_timestep_nr++;

			this->do_output();

			if (this->should_quit())
				break;
		}

	}


public:

	/**
	 * ODE to simulate
	 */
	double f_dt(double y, double t)
	{
		double a = param_parareal_function_a;
		double b = param_parareal_function_b;

#if 0
		double y0 = param_parareal_function_y0;
		return b*sin(t) / (1 - a*sin(y0));
#else
		return a * std::sin(y) + b * std::sin(t);
#endif
	}


	bool should_quit()
	{
		if (
				simVars->timecontrol.max_timesteps_nr != -1 &&
				simVars->timecontrol.max_timesteps_nr <= simVars->timecontrol.current_timestep_nr
		)
			return true;

		if (!std::isinf(simVars->timecontrol.max_simulation_time))
			if (simVars->timecontrol.max_simulation_time <= simVars->timecontrol.current_simulation_time+simVars->timecontrol.max_simulation_time*1e-10)	// care about roundoff errors with 1e-10
				return true;

		return false;
	}


	void do_output()
	{
		char buffer[1024];

		const char* filename_template = "output_%s_t%020.8f.csv";
		sprintf(buffer, filename_template, "prog_u", simVars->timecontrol.current_simulation_time);

		std::ofstream file(buffer, std::ios_base::trunc);
		file << std::setprecision(16);

		file << "#SWEET" << std::endl;
		file << "#FORMAT ASCII" << std::endl;
		file << "#PRIMITIVE SCALAR" << std::endl;

		file << this->prog_u;

		file.close();
	}


};



int main(int i_argc, char *i_argv[])
{
	const char *bogus_var_names[] = {
		"parareal-fine-dt",
		"parareal-function-param-y0",
		"parareal-function-param-a",
		"parareal-function-param-b",
		nullptr
	};

	param_parareal_fine_dt = -1;
	param_parareal_function_y0 = 0.123;
	param_parareal_function_a = 1.0;
	param_parareal_function_b = 0.1;

	if (!simVars.setupFromMainParameters(i_argc, i_argv, bogus_var_names))
	{
		///std::cout << "	--parareal-fine-dt				Fine time stepping size" << std::endl;
		std::cout << "	--parareal-function-param-y0	Parameter 'y0' (initial condition) for function y(t=0)" << std::endl;
		std::cout << "	--parareal-function-param-a		Parameter 'a' for function 'a*sin(y) + b*sin(t)" << std::endl;
		std::cout << "	--parareal-function-param-b		Parameter 'b' for function 'a*sin(y) + b*sin(t)" << std::endl;
		std::cout << "Unknown option detected" << std::endl;
		exit(-1);
	}


	param_parareal_fine_dt = simVars.timecontrol.current_timestep_size;
	//if (simVars.bogus.var[0] != "")
	//	param_parareal_fine_dt = atof(simVars.bogus.var[0].c_str());
	if (simVars.bogus.var[1] != "")
		param_parareal_function_y0 = atof(simVars.bogus.var[1].c_str());
	if (simVars.bogus.var[2] != "")
		param_parareal_function_a = atof(simVars.bogus.var[2].c_str());
	if (simVars.bogus.var[3] != "")
		param_parareal_function_b = atof(simVars.bogus.var[3].c_str());

	if (param_parareal_fine_dt <= 0)
	{
		std::cout << "Specify fine time step size via --parareal-fine-dt=[value]" << std::endl;
		return -1;
	}

#if !SWEET_PARAREAL

	SimulationInstance* sim = new SimulationInstance(&simVars);

	sim->run();

	delete sim;

#endif

#if SWEET_PARAREAL
	if (!simVars.parareal.enabled)
	{
		std::cout << "Activate parareal mode via --parareal0enable=1" << std::endl;
		return -1;
	}

	std::cout << "Running parareal" << std::endl;
	std::cout << " + fine dt: " << param_parareal_fine_dt << std::endl;
	std::cout << " + initial condition: y0=" << param_parareal_function_y0 << std::endl;
	std::cout << " + function df(y,t)/dt = " << param_parareal_function_a << "*sin(y) + " << param_parareal_function_b << "*sin(t)" << std::endl;

	if (simVars.parareal.enabled)
	{

		ODE_Scalar_TimeSteppers* timeSteppersFine = new ODE_Scalar_TimeSteppers;
		ODE_Scalar_TimeSteppers* timeSteppersCoarse = new ODE_Scalar_TimeSteppers;

		/*
		 * Allocate parareal controller and provide class
		 * which implement the parareal features
		 */
		Parareal_Controller_Serial_GenericData<ODE_Scalar_TimeSteppers, 1> parareal_Controller_Serial(simVars,
															timeSteppersFine,
															timeSteppersCoarse);
		//Parareal_Controller_Serial<SimulationInstance> parareal_Controller_Serial;

		// setup controller. This initializes several simulation instances
		//parareal_Controller_Serial.setup(&simVars.parareal);
		parareal_Controller_Serial.setup();

		// execute the simulation
		parareal_Controller_Serial.run();

		delete timeSteppersFine;
		delete timeSteppersCoarse;
	}
#endif


	return 0;
}
