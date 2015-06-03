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
#define STATE_DATE_DELTA_TYPE "delta_type"
#define STATE_DATE_DELTA_FIXED "delta_fixed"
#define STATE_DATE_DELTA_MIN "delta_min"
#define STATE_DATE_DELTA_MAX "delta_max"
#define STATE_DATE_DELTA_AVERAGE "delta_average"
#define STATE_DATE_DELTA_ERROR "delta_error"

#define STATE_EVENT_TYPE "type"
#define STATE_EVENT_METHOD "method"
#define STATE_EVENT_DATES "dates"
#define STATE_EVENT_PHASE_IDS "phase_ids"

#define STATE_EVENT_KNOWN_TYPE "known_type"
#define STATE_EVENT_KNOWN_FIXED "known_fixed"
#define STATE_EVENT_KNOWN_START "known_unif_start"
#define STATE_EVENT_KNOWN_END "known_unif_end"

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

#endif
