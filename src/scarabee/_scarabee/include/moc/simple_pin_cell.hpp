#ifndef SIMPLE_PIN_CELL_H
#define SIMPLE_PIN_CELL_H

#include <moc/cell.hpp>
#include <moc/surface.hpp>
#include <moc/pin_cell_type.hpp>
#include <data/cross_section.hpp>

#include <cereal/cereal.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/base_class.hpp>

#include <memory>

namespace scarabee {

class SimplePinCell : public Cell {
 public:
  SimplePinCell(const std::vector<double>& rads,
                const std::vector<std::shared_ptr<CrossSection>>& mats,
                double dx, double dy, PinCellType pin_type = PinCellType::Full);

 private:
  std::vector<double> mat_radii_;
  std::vector<std::shared_ptr<CrossSection>> mats_;
  std::vector<std::shared_ptr<Surface>> radii_;
  PinCellType pin_type_;

  friend class cereal::access;
  SimplePinCell() {}
  template <class Archive>
  void serialize(Archive& arc) {
    arc(cereal::base_class<Cell>(this), CEREAL_NVP(mat_radii_),
        CEREAL_NVP(mats_), CEREAL_NVP(radii_), CEREAL_NVP(pin_type_));
  }

  void build_full();
  void build_xn();
  void build_xp();
  void build_yn();
  void build_yp();
  void build_i();
  void build_ii();
  void build_iii();
  void build_iv();
};

}  // namespace scarabee

#endif
