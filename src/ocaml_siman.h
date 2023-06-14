#include <stdio.h>
#include <string.h> // for memcpy

// Prevent OCaml from exporting short macro names.
#define CAML_NAME_SPACE 1

#include <caml/callback.h>
#include <caml/mlvalues.h>
#include <caml/memory.h> // CAMLreturn
#include <caml/alloc.h> // caml_copy
#include <caml/signals.h> // control the runtime

#include <gsl_errno.h>
#include <gsl_math.h> // log1p
#include <gsl_statistics_double.h>
#include <gsl_sf_erf.h>
#include <gsl_fit.h> // gsl_fit_linear
#include <gsl_integration.h>
#include <gsl_multifit_nlinear.h>
#include <gsl_cdf.h>
#include <gsl_sf_gamma.h> // gsl_sf_fact
#include <gsl_randist.h> // gsl_ran_binomial_pdf

#include <siman.h> // simulated annealing
#include <gsl_rng.h> // random number generator

struct ocaml_siman_object {
  value state;
};

double ocaml_siman_energy (struct ocaml_siman_object* xp) {
  CAMLparam0 ();
  CAMLlocal1 (energy);
  energy = Field (Field (xp->state, 0), 1);
  CAMLreturnT (double, Double_val (caml_callback (energy, xp->state)));
}

void ocaml_siman_step (
  const gsl_rng* rng,
  struct ocaml_siman_object* xp,
  double step_size
) {
  CAMLparam0 ();
  CAMLlocal1 (step);
  step = Field (Field (xp->state, 0), 2);
  xp->state = caml_callback2 (step, xp->state, caml_copy_double (step_size));
  CAMLreturn0;
}

double ocaml_siman_dist (
  struct ocaml_siman_object* xp,
  struct ocaml_siman_object* yp
) {
  CAMLparam0 ();
  CAMLlocal1 (dist);
  dist = Field (Field (xp->state, 0), 3);
  CAMLreturnT (double, Double_val (caml_callback2 (dist, xp->state, yp->state)));
}

void ocaml_siman_copy_into (
  struct ocaml_siman_object* xp,
  struct ocaml_siman_object* yp
) {
  CAMLparam0 ();
  CAMLlocal2 (src, copy);
  src = xp->state;
  copy = Field (Field (src, 0), 0);
  yp->state = caml_callback (copy, src);
  CAMLreturn0;
}

void* ocaml_siman_copy (struct ocaml_siman_object* xp) {
  CAMLparam0 ();
  CAMLlocal2 (src, copy);
  src = xp->state;
  copy = Field (Field (src, 0), 0);
  struct ocaml_siman_object* yp = malloc (sizeof (struct ocaml_siman_object));
  caml_register_global_root (&(yp->state));
  yp->state = caml_callback (copy, src);
  CAMLreturnT (struct ocaml_siman_object*, yp);
}

void ocaml_siman_destroy (struct ocaml_siman_object* xp) {
//   CAMLparam0 ();
  caml_remove_global_root (&(xp->state));
  free (xp);
//   CAMLreturn0;
}

void ocaml_siman_print (struct ocaml_siman_object* xp) {
  CAMLparam0 ();
  CAMLlocal1 (print);
  print = Field (Field (xp->state, 0), 4);
  if (Is_some (print)) {
    caml_callback (Field (print, 0), xp->state);
  }
  CAMLreturn0;
}

CAMLprim value ocaml_siman_solve (value num_iters, value step_size, value state){
  CAMLparam3 (num_iters, step_size, state);
  CAMLlocal1 (res);

  struct ocaml_siman_object* xp = malloc (sizeof (struct ocaml_siman_object));
  xp->state = state;
  caml_register_global_root (&(xp->state));

  gsl_rng_env_setup ();
  gsl_rng* rng = gsl_rng_alloc (gsl_rng_default);

  gsl_siman_params_t params = {
    .n_tries = 200, // 200
    .iters_fixed_T = Int_val (num_iters), // 1000
    .step_size = Double_val (step_size), // 1
    .k = 1.0, // Boltzman constant
    .t_initial = 0.008, // initial temperature
    .mu_t = 1.003,
    .t_min = 2.0e-6 // minimum temperature
  };

  gsl_siman_print_t printer = NULL;
/*
  if (Is_some (Field (Field (xp->state, 0), 4))) {
    printer = (gsl_siman_print_t) ocaml_siman_print;
  }
*/
  siman_solve (
    rng,
    (void*) xp,
    (gsl_siman_Efunc_t) ocaml_siman_energy,
    (gsl_siman_step_t) ocaml_siman_step,
    (gsl_siman_metric_t) ocaml_siman_dist,
    printer,
    (gsl_siman_copy_t) ocaml_siman_copy_into,
    (gsl_siman_copy_construct_t) ocaml_siman_copy,
    (gsl_siman_destroy_t) ocaml_siman_destroy,
    0, // element size - signal variable.
    params
  );
  gsl_rng_free (rng);

  res = Field (xp->state, 1);
  caml_remove_global_root (&(xp->state));
  free (xp);

  CAMLreturn (res);
}
