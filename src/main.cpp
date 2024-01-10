#include <cylindrical_cell.hpp>
#include <cylindrical_flux_solver.hpp>
#include <utils/constants.hpp>

#include <cmath>
#include <iostream>
#include <memory>
#include <vector>

int main() {
  std::shared_ptr<MGCrossSections> UO2 = std::make_shared<MGCrossSections>();
  UO2->fissile_ = true;
  UO2->Etr_ = {1.77949E-01, 3.29805E-01, 4.80388E-01, 5.54367E-01, 3.11801E-01, 3.95168E-01, 5.64406E-01};
  UO2->Et_ = UO2->Etr_;
  UO2->Ea_  = {8.02480E-03, 3.71740E-03, 2.67690E-02, 9.62360E-02, 3.00200E-02, 1.11260E-01, 2.82780E-01};
  UO2->Ef_  = {7.21206E-03, 8.19301E-04, 6.45320E-03, 1.85648E-02, 1.78084E-02, 8.30348E-02, 2.16004E-01};
  UO2->nu_  = NDArray<double>({2.78145, 2.47443, 2.43383, 2.43380, 2.43380, 2.43380, 2.43380}, {1, 7});
  UO2->chi_ = NDArray<double>({5.87910E-01, 4.11760E-01, 3.39060E-04, 1.17610E-07, 0., 0., 0.}, {1, 7});
  UO2->Es_tr_ = NDArray<double>({1.27537E-01, 4.23780E-02, 9.43740E-06, 5.51630E-09, 0.00000E+00, 0.00000E+00, 0.00000E+00,
                                 0.00000E+00, 3.24456E-01, 1.63140E-03, 3.14270E-09, 0.00000E+00, 0.00000E+00, 0.00000E+00,
                                 0.00000E+00, 0.00000E+00, 4.50940E-01, 2.67920E-03, 0.00000E+00, 0.00000E+00, 0.00000E+00,
                                 0.00000E+00, 0.00000E+00, 0.00000E+00, 4.52565E-01, 5.56640E-03, 0.00000E+00, 0.00000E+00,
                                 0.00000E+00, 0.00000E+00, 0.00000E+00, 1.25250E-04, 2.71401E-01, 1.02550E-02, 1.00210E-08,
                                 0.00000E+00, 0.00000E+00, 0.00000E+00, 0.00000E+00, 1.29680E-03, 2.65802E-01, 1.68090E-02,
                                 0.00000E+00, 0.00000E+00, 0.00000E+00, 0.00000E+00, 0.00000E+00, 8.54580E-03, 2.73080E-01}, {7, 7});

  std::shared_ptr<MGCrossSections> H2O = std::make_shared<MGCrossSections>();
  H2O->fissile_ = false;
  H2O->Etr_ = {1.59206E-01, 4.12970E-01, 5.90310E-01, 5.84350E-01, 7.18000E-01, 1.25445E+00, 2.65038E+00};
  H2O->Et_ = H2O->Etr_;
  H2O->Ea_  = {6.01050E-04, 1.57930E-05, 3.37160E-04, 1.94060E-03, 5.74160E-03, 1.50010E-02, 3.72390E-02};
  H2O->Es_tr_  = NDArray<double>({4.44777E-02, 1.13400E-01, 7.23470E-04, 3.74990E-06, 5.31840E-08, 0.00000E+00, 0.00000E+00,
                                  0.00000E+00, 2.82334E-01, 1.29940E-01, 6.23400E-04, 4.80020E-05, 7.44860E-06, 1.04550E-06,
                                  0.00000E+00, 0.00000E+00, 3.45256E-01, 2.24570E-01, 1.69990E-02, 2.64430E-03, 5.03440E-04,
                                  0.00000E+00, 0.00000E+00, 0.00000E+00, 9.10284E-02, 4.15510E-01, 6.37320E-02, 1.21390E-02,
                                  0.00000E+00, 0.00000E+00, 0.00000E+00, 7.14370E-05, 1.39138E-01, 5.11820E-01, 6.12290E-02,
                                  0.00000E+00, 0.00000E+00, 0.00000E+00, 0.00000E+00, 2.21570E-03, 6.99913E-01, 5.37320E-01,
                                  0.00000E+00, 0.00000E+00, 0.00000E+00, 0.00000E+00, 0.00000E+00, 1.32440E-01, 2.48070E+00}, {7, 7});

  // Vector of all the radii
  std::vector<double> radii                          {0.1, 0.2, 0.3, 0.4, 0.45, 0.5, 0.54, 0.58, 0.61, 0.65, 1.26/std::sqrt(PI)};
  std::vector<std::shared_ptr<MGCrossSections>> mats {UO2, UO2, UO2, UO2, UO2,  UO2, UO2,  H2O,  H2O,  H2O,  H2O};

  std::shared_ptr<CylindricalCell> cell = std::make_shared<CylindricalCell>(radii, mats);
  std::cout << ">>> Solving for collision probabilities...\n";
  cell->solve();
  std::cout << ">>> Collision probabilities determined !\n";
  std::cout << ">>> Solving for the flux...\n";

  CylindricalFluxSolver cell_flux(cell);
  cell_flux.solve();

  return 0;
}
