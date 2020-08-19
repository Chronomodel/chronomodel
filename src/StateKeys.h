/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2018

Authors :
	Philippe LANOS
	Helori LANOS
 	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This software is governed by the CeCILL V2.1 license under French law and
abiding by the rules of distribution of free software.  You can  use,
modify and/ or redistribute the software under the terms of the CeCILL
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info".

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's author,  the holder of the
economic rights,  and the successive licensors  have only  limited
liability.

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or
data to be ensured and,  more generally, to use and operate it in the
same conditions as regards security.

The fact that you are presently reading this means that you have had
knowledge of the CeCILL V2.1 license and that you accept its terms.
--------------------------------------------------------------------- */

#ifndef STATEKEYS_H
#define STATEKEYS_H

#define STATE_APP_VERSION "app_version"
#define STATE_SETTINGS "settings"
#define STATE_MCMC "mcmc"
#define STATE_EVENTS "events"
#define STATE_PHASES "phases"
#define STATE_EVENTS_CONSTRAINTS "events_constraints"
#define STATE_PHASES_CONSTRAINTS "phases_constraints"
#define STATE_DATES_TRASH "dates_trash"
#define STATE_EVENTS_TRASH "events_trash"

#define STATE_RUN "run"
#define STATE_RESULTS_FILE "results_file"

#define STATE_SETTINGS_TMIN "tmin"
#define STATE_SETTINGS_TMAX "tmax"
#define STATE_SETTINGS_STEP "step"
#define STATE_SETTINGS_STEP_FORCED "step_forced"
#define STATE_SETTINGS_FORMAT_DATE "format_date"

#define STATE_ID "id"
#define STATE_NAME "name"
#define STATE_COLOR_RED "red"
#define STATE_COLOR_GREEN "green"
#define STATE_COLOR_BLUE "blue"
#define STATE_ITEM_X "item_x"
#define STATE_ITEM_Y "item_y"
#define STATE_IS_SELECTED "is_selected"
#define STATE_IS_CURRENT "is_current"

#define STATE_DATE_DATA "data"
#define STATE_DATE_PLUGIN_ID "plugin_id"
#define STATE_DATE_METHOD "method"
#define STATE_DATE_VALID "valid"
#define STATE_DATE_ORIGIN "origin"
#define STATE_DATE_DELTA_TYPE "delta_type"
#define STATE_DATE_DELTA_FIXED "delta_fixed"
#define STATE_DATE_DELTA_MIN "delta_min"
#define STATE_DATE_DELTA_MAX "delta_max"
#define STATE_DATE_DELTA_AVERAGE "delta_average"
#define STATE_DATE_DELTA_ERROR "delta_error"
#define STATE_DATE_SUB_DATES "subdates"

#define STATE_EVENT_TYPE "type"
#define STATE_EVENT_METHOD "method"
#define STATE_EVENT_DATES "dates"
#define STATE_EVENT_PHASE_IDS "phase_ids"

//#define STATE_EVENT_KNOWN_TYPE "known_type"
#define STATE_EVENT_KNOWN_FIXED "known_fixed"
//#define STATE_EVENT_KNOWN_START "known_unif_start"
//#define STATE_EVENT_KNOWN_END "known_unif_end"

#define STATE_PHASE_TAU_TYPE "tau_type"
#define STATE_PHASE_TAU_FIXED "tau_fixed"
#define STATE_PHASE_TAU_MIN "tau_min"
#define STATE_PHASE_TAU_MAX "tau_max"

#define STATE_CONSTRAINT_BWD_ID "bwd_id"
#define STATE_CONSTRAINT_FWD_ID "fwd_id"

#define STATE_CONSTRAINT_GAMMA_TYPE "gamma_type"
#define STATE_CONSTRAINT_GAMMA_FIXED "gamma_fixed"
#define STATE_CONSTRAINT_GAMMA_MIN "gamma_min"
#define STATE_CONSTRAINT_GAMMA_MAX "gamma_max"

#define STATE_MCMC_NUM_CHAINS "num_proc"
#define STATE_MCMC_NUM_RUN_ITER "num_iter"
#define STATE_MCMC_NUM_BURN_ITER "num_burn"
#define STATE_MCMC_MAX_ADAPT_BATCHES "max_batches"
#define STATE_MCMC_ITER_PER_BATCH "iter_per_batch"
#define STATE_MCMC_THINNING_INTERVAL "thinning_interval"
#define STATE_MCMC_FFT_LEN "fft_len"
#define STATE_MCMC_CALIB_STEP "calib_step"
#define STATE_MCMC_SEEDS "seeds"

#define STATE_MCMC_MIXING "mixing_level"

#endif
