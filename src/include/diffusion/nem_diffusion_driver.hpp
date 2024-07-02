#ifndef SCARABEE_NEM_DIFFUSION_DRIVER_H
#define SCARABEE_NEM_DIFFUSION_DRIVER_H

#include <diffusion_cross_section.hpp>
#include <diffusion/diffusion_geometry.hpp>

#include <Eigen/Dense>

#include <xtensor/xtensor.hpp>

#include <cmath>
#include <memory>
#include <tuple>

namespace scarabee {

class NEMDiffusionDriver {
 public:
  NEMDiffusionDriver(std::shared_ptr<DiffusionGeometry> geom);

  std::size_t ngroups() const { return geom_->ngroups(); }

  void solve();
  bool solved() const { return solved_; }

  double keff_tolerance() const { return keff_tol_; }
  void set_keff_tolerance(double ktol);

  double flux_tolerance() const { return flux_tol_; }
  void set_flux_tolerance(double ftol);

  double keff() const { return keff_; }

  double flux(double x, double y, double z, std::size_t g) const;
  xt::xtensor<double, 4> flux(const xt::xtensor<double, 1>& x,
                              const xt::xtensor<double, 1>& y,
                              const xt::xtensor<double, 1>& z) const;
  xt::xtensor<double, 4> avg_flux() const;

  double power(double x, double y, double z) const;
  xt::xarray<double> power(xt::xarray<double> x, xt::xarray<double> y,
                           xt::xarray<double> z) const;

 private:
  //----------------------------------------------------------------------------
  // TYPES
  // Definition of coupling matrix types
  using Current = Eigen::Matrix<double, 6, 1>;
  enum CurrentIndx : ::std::size_t {
    XP = 0,
    XM = 1,
    YP = 2,
    YM = 3,
    ZP = 4,
    ZM = 5
  };

  using MomentsVector = Eigen::Matrix<double, 7, 1>;
  enum MomentIndx : ::std::size_t {
    AVG = 0,
    X1 = 1,
    Y1 = 2,
    Z1 = 3,
    X2 = 4,
    Y2 = 5,
    Z2 = 6
  };

  using RMat = Eigen::Matrix<double, 6, 6>;
  using PMat = Eigen::Matrix<double, 6, 7>;

  //----------------------------------------------------------------------------
  // PRIVATE MEMBERS
  std::shared_ptr<DiffusionGeometry> geom_;
  const std::size_t NG_;  // Number of groups
  const std::size_t NM_;  // Number of regions

  // Quantities required for reconstructing the flux  (kept after solution)
  xt::xtensor<double, 2> flux_avg_;  // First index is group, second is node
  xt::xtensor<double, 2> flux_x1_;
  xt::xtensor<double, 2> flux_x2_;
  xt::xtensor<double, 2> flux_y1_;
  xt::xtensor<double, 2> flux_y2_;
  xt::xtensor<double, 2> flux_z1_;
  xt::xtensor<double, 2> flux_z2_;
  xt::xtensor<Current, 2> j_outs_;
  xt::xtensor<Current, 2> j_ins_;

  // Quantites used for calculation (not needed for reconstruction)
  xt::xtensor<RMat, 2> Rmats_;  // First index is group, second is node
  xt::xtensor<PMat, 2> Pmats_;
  xt::xtensor<MomentsVector, 2> Q_;  // Source

  double keff_ = 1.;
  double flux_tol_ = 1.E-5;
  double keff_tol_ = 1.E-5;
  bool solved_{false};

  //----------------------------------------------------------------------------
  // PRIVATE METHODS
  void fill_coupling_matrices();
  void fill_source();
  void update_Jin_from_Jout(std::size_t g, std::size_t m);
  MomentsVector calc_leakage_moments(std::size_t g, std::size_t m) const;
  double calc_keff(double keff, const xt::xtensor<double, 2>& old_flux,
                   const xt::xtensor<double, 2>& new_flux) const;
  void calc_node(const std::size_t g, const std::size_t m,
                 const xt::svector<std::size_t>& geom_indx,
                 const double invs_dx, const double invs_dy,
                 const double invs_dz, const DiffusionCrossSection& xs);
  void inner_iteration();

  inline double calc_net_current(const Current& Jin, const Current& Jout,
                                 CurrentIndx indx) const {
    if (indx == CurrentIndx::XP || indx == CurrentIndx::YP ||
        indx == CurrentIndx::ZP) {
      return Jout(indx) - Jin(indx);
    } else {
      return Jin(indx) - Jout(indx);
    }
  }

  inline double f0(double xi) const { return 1.; }

  inline double f1(double xi) const { return xi; }

  inline double f2(double xi) const { return 3. * xi * xi - 0.25; }

  inline double f3(double xi) const { return xi * (xi - 0.5) * (xi + 0.5); }

  inline double f4(double xi) const {
    return (xi * xi - 0.05) * (xi - 0.5) * (xi + 0.5);
  }

  struct Unity {
    double operator()(double /*k*/, double /*x*/) const { return 1.; }

    double diff(double /*k*/, double /*x*/) const { return 0.; }

    double intgr(double /*k*/, double del) const {
      // Integrate from -del/2 to del/2
      return del;
    }
  };

  struct Cosh {
    double operator()(double k, double x) const { return std::cosh(k * x); }

    double diff(double k, double x) const { return k * std::sinh(k * x); }

    double intgr(double k, double del) const {
      // Integrate from -del/2 to del/2
      return 2. * std::sinh(0.5 * k * del) / k;
    }
  };

  struct Sinh {
    double operator()(double k, double x) const { return std::sinh(k * x); }

    double diff(double k, double x) const { return k * std::cosh(k * x); }

    double intgr(double /*k*/, double /*del*/) const {
      // Integrate from -del/2 to del/2
      return 0.;
    }
  };

  template <class Fx, class Fy>
  struct F {
    Fx fx;
    Fy fy;

    double operator()(double kx, double ky, double x, double y) const {
      return fx(kx, x) * fy(ky, y);
    }

    double diffx_intgry(double kx, double ky, double dely, double x) const {
      return fx.diff(kx, x) * fy.intgr(ky, dely);
    }

    double diffy_intgrx(double kx, double ky, double delx, double y) const {
      return fx.intgr(kx, delx) * fy.diff(ky, y);
    }

    double intgr(double kx, double ky, double delx, double dely) const {
      return fx.intgr(ky, delx) * fy.intgr(ky, dely);
    }
  };

  using F00 = F<Unity, Unity>;
  using F01 = F<Unity, Sinh>;
  using F02 = F<Unity, Cosh>;
  using F10 = F<Sinh, Unity>;
  using F11 = F<Sinh, Sinh>;
  using F12 = F<Sinh, Cosh>;
  using F20 = F<Cosh, Unity>;
  using F21 = F<Cosh, Sinh>;
  using F22 = F<Cosh, Cosh>;
};

}  // namespace scarabee

#endif