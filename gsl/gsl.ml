external ocaml_log1p : float -> float = "ocaml_log1p"

module Linear_fit = struct
  type t = {
    c0: float;
    c1: float;
  }
  external ocaml_gsl_fit_linear : float array -> float array -> t = "ocaml_gsl_fit_linear"

  let f = ocaml_gsl_fit_linear
end

module Integrate = struct
  type t = {
    out: float;
    err: float;
    neval: int64;
  }
  external ocaml_integrate : (float -> float) -> float -> float -> t = "ocaml_integrate"

  let f = ocaml_integrate
end
