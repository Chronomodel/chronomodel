/* ---------------------------------------------------------------------

Copyright © CNRS	2014 - 2026

Authors :
	Philippe LANOS
	Helori LANOS
	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.

SPDX-License-Identifier: GPL-3.0-or-later
--------------------------------------------------------------------- */

#ifndef STATEKEYS_H
#define STATEKEYS_H

#define STATE_APP_VERSION "app_version"
#define STATE_SETTINGS "settings"
#define STATE_MCMC "mcmc"

#define STATE_EVENTMODEL "event_model"
#define STATE_FORMATDATE "format_date"
#define STATE_FORMATDATE_CUSTOM "format_date_custom"


#define STATE_CURVE "curve"
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

#define STATE_DATE_UUID "UUID"
#define STATE_DATE_DATA "data"
#define STATE_DATE_PLUGIN_ID "plugin_id"
#define STATE_DATE_METHOD "method" // Obsolete change with "sampler"
#define STATE_DATE_SAMPLER "sampler"

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
#define STATE_EVENT_METHOD "method" // Obsolete change with "sampler"
#define STATE_EVENT_SAMPLER "sampler"

#define STATE_EVENT_DATES "dates"
#define STATE_EVENT_PHASE_IDS "phase_ids"

#define STATE_EVENT_KNOWN_FIXED "known_fixed"

#define STATE_EVENT_POINT_TYPE "point_type"  // default or node
#define STATE_EVENT_X_INC_DEPTH "X_Inc_Depth"   // old value "YInc"
#define STATE_EVENT_SX_ALPHA95_SDEPTH "sX_Alpha95_sDepth" // old value"SInc"

#define STATE_EVENT_Y_DEC "Y_Dec" //old value "YDec"
#define STATE_EVENT_SY "sY" // old value "SDec"

#define STATE_EVENT_Z_F "Z_Field" // old value"YInt"
#define STATE_EVENT_SZ_SF "sZ_sF" // old value "SInt"

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


// ----- Paramètres d’annealing -----
#define STATE_MCMC_ANNEAL_TEMP  "anneal_temp"
#define STATE_MCMC_ANNEAL_RECURRENCE "anneal_recurrence"
#define STATE_MCMC_ANNEAL_DWELL "anneal_dwell"

#define STATE_CURVE_PROCESS_TYPE "process_type"

#define STATE_CURVE_THRESHOLD "speed_threshold"

#define STATE_CURVE_USE_ERR_MESURE "use_err_mesure"

#define STATE_CURVE_TIME_TYPE "time_type"
#define STATE_CURVE_VARIANCE_TYPE "variance_type"
#define STATE_CURVE_USE_VARIANCE_INDIVIDUAL "variance_individual"
#define STATE_CURVE_VARIANCE_FIXED "variance_fixed"
#define STATE_CURVE_COEFF_LISSAGE_TYPE "coeff_lissage_type"
#define STATE_CURVE_ALPHA_LISSAGE "alpha"

#endif
