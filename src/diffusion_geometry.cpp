#include <diffusion/diffusion_geometry.hpp>
#include <utils/logging.hpp>
#include <utils/scarabee_exception.hpp>

#include <xtensor/xstrides.hpp>

#include <algorithm>
#include <sstream>

namespace scarabee {

DiffusionGeometry::DiffusionGeometry(const std::vector<TileFill>& tiles,
                                     const std::vector<double>& dx,
                                     const std::vector<std::size_t>& xdivs,
                                     double albedo_xn, double albedo_xp)
    : tiles_(),
      xn_(),
      xp_(),
      yn_(),
      yp_(),
      zn_(),
      zp_(),
      tile_dx_(dx),
      x_divs_per_tile_(xdivs),
      tile_dy_(),
      y_divs_per_tile_(),
      tile_dz_(),
      z_divs_per_tile_(),
      nmats_(0),
      mat_indx_to_flat_geom_indx_(),
      nx_(0),
      ny_(0),
      nz_(0),
      geom_shape_() {
  // Make sure numbers are coherent !
  if ((tile_dx_.size() != x_divs_per_tile_.size()) ||
      (tile_dx_.size() != tiles.size())) {
    auto mssg =
        "The number of provided tiles, widths, and divisions does not agree.";
    spdlog::error(mssg);
    throw ScarabeeException(mssg);
  }

  if (tile_dx_.size() == 0) {
    auto mssg = "No tiles were provided.";
    spdlog::error(mssg);
    throw ScarabeeException(mssg);
  }

  // Make sure all widths > 0
  for (std::size_t i = 0; i < tile_dx_.size(); i++) {
    if (tile_dx_[i] <= 0.) {
      std::stringstream mssg;
      mssg << "dx at index " << i << " is <= 0.";
      spdlog::error(mssg.str());
      throw ScarabeeException(mssg.str());
    }
  }

  // Make sure all divs are > 0
  nx_ = 0;
  for (std::size_t i = 0; i < x_divs_per_tile_.size(); i++) {
    nx_ += x_divs_per_tile_[i];
    if (x_divs_per_tile_[i] == 0) {
      std::stringstream mssg;
      mssg << "xdivs at index " << i << " is zero.";
      spdlog::error(mssg.str());
      throw ScarabeeException(mssg.str());
    }
  }

  // We can now assign the geometry shape
  geom_shape_ = {nx_};

  // Assign boundary conditions
  if (albedo_xn < 0. || albedo_xn > 1.) {
    std::stringstream mssg;
    mssg << "Negative x albedo is invalid. Must be in range [0, 1].";
    spdlog::error(mssg.str());
    throw ScarabeeException(mssg.str());
  }
  xn_.albedo = albedo_xn;
  xn_.xs = nullptr;

  if (albedo_xp < 0. || albedo_xp > 1.) {
    std::stringstream mssg;
    mssg << "Positive x albedo is invalid. Must be in range [0, 1].";
    spdlog::error(mssg.str());
    throw ScarabeeException(mssg.str());
  }
  xp_.albedo = albedo_xp;
  xp_.xs = nullptr;

  // Reshape tiles array
  tiles_.resize({tile_dx_.size()});

  // Assign all tiles
  for (std::size_t i = 0; i < tiles.size(); i++) {
    if (std::holds_alternative<double>(tiles[i])) {
      std::stringstream mssg;
      mssg << "A 1D diffusion problem cannot have albedo tiles.";
      spdlog::error(mssg.str());
      throw ScarabeeException(mssg.str());
    } else {
      tiles_(i).xs = std::get<std::shared_ptr<DiffusionCrossSection>>(tiles[i]);
    }
  }

  // Since a 1D problem cannot have albedo tiles, then we know all divs are
  // materials. Therefore, the material index m is equivalen to the flat
  // geometry index. This fact greatly simplifies the remaining bits.
  nmats_ = nx_;
  mat_indx_to_flat_geom_indx_.resize(nmats_);
  for (std::size_t m = 0; m < nmats_; m++) mat_indx_to_flat_geom_indx_[m] = m;

  // We must make sure mat_indx_to_flat_geom is sorted, otherwise our method
  // to get mat index will not work !
  if (std::is_sorted(mat_indx_to_flat_geom_indx_.begin(),
                     mat_indx_to_flat_geom_indx_.end()) == false) {
    auto mssg = "Material index to flat geometry index is not sorted.";
    spdlog::error(mssg);
    throw ScarabeeException(mssg);
  }
}

DiffusionGeometry::DiffusionGeometry(const std::vector<TileFill>& tiles,
                                     const std::vector<double>& dx,
                                     const std::vector<std::size_t>& xdivs,
                                     const std::vector<double>& dy,
                                     const std::vector<std::size_t>& ydivs,
                                     double albedo_xn, double albedo_xp,
                                     double albedo_yn, double albedo_yp)
    : tiles_(),
      xn_(),
      xp_(),
      yn_(),
      yp_(),
      zn_(),
      zp_(),
      tile_dx_(dx),
      x_divs_per_tile_(xdivs),
      tile_dy_(dy),
      y_divs_per_tile_(ydivs),
      tile_dz_(),
      z_divs_per_tile_(),
      nmats_(0),
      mat_indx_to_flat_geom_indx_(),
      nx_(0),
      ny_(0),
      nz_(0),
      geom_shape_() {
  // Make sure numbers are coherent !
  if ((tile_dx_.size() != x_divs_per_tile_.size()) ||
      (tile_dy_.size() != y_divs_per_tile_.size())) {
    auto mssg = "The number of provided widths and divisions do not agree.";
    spdlog::error(mssg);
    throw ScarabeeException(mssg);
  }

  if (tiles.size() != tile_dx_.size() * tile_dy_.size()) {
    auto mssg =
        "The number of provided tiles does not agree with the number of "
        "divisions.";
    spdlog::error(mssg);
    throw ScarabeeException(mssg);
  }

  if (tile_dx_.size() == 0) {
    auto mssg = "No tiles were provided along x.";
    spdlog::error(mssg);
    throw ScarabeeException(mssg);
  }
  if (tile_dy_.size() == 0) {
    auto mssg = "No tiles were provided along y.";
    spdlog::error(mssg);
    throw ScarabeeException(mssg);
  }

  // Make sure all widths > 0
  for (std::size_t i = 0; i < tile_dx_.size(); i++) {
    if (tile_dx_[i] <= 0.) {
      std::stringstream mssg;
      mssg << "dx at index " << i << " is <= 0.";
      spdlog::error(mssg.str());
      throw ScarabeeException(mssg.str());
    }
  }
  for (std::size_t i = 0; i < tile_dy_.size(); i++) {
    if (tile_dy_[i] <= 0.) {
      std::stringstream mssg;
      mssg << "dy at index " << i << " is <= 0.";
      spdlog::error(mssg.str());
      throw ScarabeeException(mssg.str());
    }
  }

  // Make sure all divs are > 0
  nx_ = 0;
  for (std::size_t i = 0; i < x_divs_per_tile_.size(); i++) {
    nx_ += x_divs_per_tile_[i];
    if (x_divs_per_tile_[i] == 0) {
      std::stringstream mssg;
      mssg << "xdivs at index " << i << " is zero.";
      spdlog::error(mssg.str());
      throw ScarabeeException(mssg.str());
    }
  }

  ny_ = 0;
  for (std::size_t i = 0; i < y_divs_per_tile_.size(); i++) {
    ny_ += y_divs_per_tile_[i];
    if (y_divs_per_tile_[i] == 0) {
      std::stringstream mssg;
      mssg << "ydivs at index " << i << " is zero.";
      spdlog::error(mssg.str());
      throw ScarabeeException(mssg.str());
    }
  }

  // We can now assign the geometry shape
  geom_shape_ = {nx_, ny_};

  // Assign boundary conditions
  if (albedo_xn < 0. || albedo_xn > 1.) {
    std::stringstream mssg;
    mssg << "Negative x albedo is invalid. Must be in range [0, 1].";
    spdlog::error(mssg.str());
    throw ScarabeeException(mssg.str());
  }
  xn_.albedo = albedo_xn;
  xn_.xs = nullptr;

  if (albedo_xp < 0. || albedo_xp > 1.) {
    std::stringstream mssg;
    mssg << "Positive x albedo is invalid. Must be in range [0, 1].";
    spdlog::error(mssg.str());
    throw ScarabeeException(mssg.str());
  }
  xp_.albedo = albedo_xp;
  xp_.xs = nullptr;

  if (albedo_yn < 0. || albedo_yn > 1.) {
    std::stringstream mssg;
    mssg << "Negative y albedo is invalid. Must be in range [0, 1].";
    spdlog::error(mssg.str());
    throw ScarabeeException(mssg.str());
  }
  yn_.albedo = albedo_yn;
  yn_.xs = nullptr;

  if (albedo_yp < 0. || albedo_yp > 1.) {
    std::stringstream mssg;
    mssg << "Positive y albedo is invalid. Must be in range [0, 1].";
    spdlog::error(mssg.str());
    throw ScarabeeException(mssg.str());
  }
  yp_.albedo = albedo_yp;
  yp_.xs = nullptr;

  // Reshape tiles array
  tiles_.resize({tile_dx_.size(), tile_dy_.size()});

  // Assign all tiles
  std::size_t tile_indx = 0;
  std::size_t num_expected_mats = 0;
  for (std::size_t j = tile_dy_.size(); j > 0; j--) {
    for (std::size_t i = 0; i < tile_dx_.size(); i++) {
      const auto& tile = tiles[tile_indx];

      if (std::holds_alternative<double>(tile)) {
        tiles_(i, j - 1).albedo = std::get<double>(tile);
      } else {
        tiles_(i, j - 1).xs =
            std::get<std::shared_ptr<DiffusionCrossSection>>(tile);
        num_expected_mats += x_divs_per_tile_[i] * y_divs_per_tile_[j - 1];
      }
      
      tile_indx++;
    }
  }

  // Assign all material indices and get their geometry index
  mat_indx_to_flat_geom_indx_.reserve(num_expected_mats);
  nmats_ = 0;
  for (std::size_t j = 0; j < ny_; j++) {
    for (std::size_t i = 0; i < nx_; i++) {
      // Create the geometry index
      const xt::svector<std::size_t> geo_inds{i, j};

      // Get the tile index
      const auto tile_inds = geom_to_tile_indx(geo_inds);

      // Grab the tile
      const auto& tile = tiles_.element(tile_inds.begin(), tile_inds.end());

      // Check if we have a material or not
      if (tile.albedo) continue;

      // If we get here, it is a material. We need to compute the flat geometry
      // index.
      std::array<xt::svector<std::size_t>, 1> geo_indx_vec{geo_inds};
      const std::size_t geom_flat_indx = xt::ravel_indices(
          geo_indx_vec, geom_shape_, xt::layout_type::column_major)[0];

      // Save to the mat_indx_to_flat_geom_indx vector
      mat_indx_to_flat_geom_indx_.push_back(geom_flat_indx);
      nmats_++;
    }
  }

  // We must make sure mat_indx_to_flat_geom is sorted, otherwise our method
  // to get mat index will not work !
  if (std::is_sorted(mat_indx_to_flat_geom_indx_.begin(),
                     mat_indx_to_flat_geom_indx_.end()) == false) {
    auto mssg = "Material index to flat geometry index is not sorted.";
    spdlog::error(mssg);
    throw ScarabeeException(mssg);
  }
}

std::size_t DiffusionGeometry::ngroups() const {
  return this->mat(0)->ngroups();
}

std::pair<DiffusionGeometry::Tile, std::optional<std::size_t>>
DiffusionGeometry::neighbor(std::size_t m, Neighbor n) const {
  if (m >= nmats()) {
    auto mssg = "Material index out of range.";
    spdlog::error(mssg);
    throw ScarabeeException(mssg);
  }

  if (ndims() == 1) {
    return neighbor_1d(m, n);
  } else if (ndims() == 2) {
    return neighbor_2d(m, n);
  } else {
    return neighbor_3d(m, n);
  }
}

std::pair<DiffusionGeometry::Tile, std::optional<std::size_t>>
DiffusionGeometry::neighbor_1d(std::size_t m, Neighbor n) const {
  if (n != Neighbor::XN && n != Neighbor::XP) {
    auto mssg = "Invalid neighbor requested for 1D geometry.";
    spdlog::error(mssg);
    throw ScarabeeException(mssg);
  }

  // First, we get the array material index
  auto geo_indx = this->geom_indx(m);

  // First, check for and edge case
  if (geo_indx[0] == 0 && n == Neighbor::XN)
    return {xn_, std::nullopt};
  else if (geo_indx[0] == nx() - 1 && n == Neighbor::XP)
    return {xp_, std::nullopt};

  // Next, change mat array to desired neighbor
  if (n == Neighbor::XN)
    geo_indx[0]--;
  else
    geo_indx[0]++;

  // Now, we get the respective tile index and tile
  xt::svector<std::size_t> tile_indx = this->geom_to_tile_indx(geo_indx);
  const auto& tile = tiles_.element(tile_indx.begin(), tile_indx.end());

  if (tile.xs == nullptr) return {tile, std::nullopt};

  // Our tile is a material. We need to get the material index.
  std::size_t mn = this->geom_to_mat_indx(geo_indx);

  return {tile, mn};
}

std::pair<DiffusionGeometry::Tile, std::optional<std::size_t>>
DiffusionGeometry::neighbor_2d(std::size_t m, Neighbor n) const {
  if (n == Neighbor::ZN || n == Neighbor::ZP) {
    auto mssg = "Invalid neighbor requested for 2D geometry.";
    spdlog::error(mssg);
    throw ScarabeeException(mssg);
  }

  // First, we get the array material index
  auto geo_indx = this->geom_indx(m);

  // First, check for and edge case
  if (geo_indx[0] == 0 && n == Neighbor::XN)
    return {xn_, std::nullopt};
  else if (geo_indx[0] == nx() - 1 && n == Neighbor::XP)
    return {xp_, std::nullopt};
  else if (geo_indx[1] == 0 && n == Neighbor::YN)
    return {yn_, std::nullopt};
  else if (geo_indx[1] == ny() - 1 && n == Neighbor::YP)
    return {yp_, std::nullopt};

  // Next, change mat array to desired neighbor
  if (n == Neighbor::XN)
    geo_indx[0]--;
  else if (n == Neighbor::XP)
    geo_indx[0]++;
  else if (n == Neighbor::YN)
    geo_indx[1]--;
  else
    geo_indx[1]++;

  // Now, we get the respective tile index and tile
  xt::svector<std::size_t> tile_indx = this->geom_to_tile_indx(geo_indx);
  const auto& tile = tiles_.element(tile_indx.begin(), tile_indx.end());

  if (tile.xs == nullptr) return {tile, std::nullopt};

  // Our tile is a material. We need to get the material index.
  std::size_t mn = this->geom_to_mat_indx(geo_indx);

  return {tile, mn};
}

std::pair<DiffusionGeometry::Tile, std::optional<std::size_t>>
DiffusionGeometry::neighbor_3d(std::size_t m, Neighbor n) const {
  // First, we get the array material index
  auto geo_indx = this->geom_indx(m);

  // First, check for and edge case
  if (geo_indx[0] == 0 && n == Neighbor::XN)
    return {xn_, std::nullopt};
  else if (geo_indx[0] == nx() - 1 && n == Neighbor::XP)
    return {xp_, std::nullopt};
  else if (geo_indx[1] == 0 && n == Neighbor::YN)
    return {yn_, std::nullopt};
  else if (geo_indx[1] == ny() - 1 && n == Neighbor::YP)
    return {yp_, std::nullopt};
  else if (geo_indx[2] == 0 && n == Neighbor::ZN)
    return {zn_, std::nullopt};
  else if (geo_indx[2] == nz() - 1 && n == Neighbor::ZP)
    return {zp_, std::nullopt};

  // Next, change mat array to desired neighbor
  if (n == Neighbor::XN)
    geo_indx[0]--;
  else if (n == Neighbor::XP)
    geo_indx[0]++;
  else if (n == Neighbor::YN)
    geo_indx[1]--;
  else if (n == Neighbor::YP)
    geo_indx[1]++;
  else if (n == Neighbor::ZN)
    geo_indx[2]--;
  else if (n == Neighbor::ZP)
    geo_indx[2]++;

  // Now, we get the respective tile index and tile
  xt::svector<std::size_t> tile_indx = this->geom_to_tile_indx(geo_indx);
  const auto& tile = tiles_.element(tile_indx.begin(), tile_indx.end());

  if (tile.xs == nullptr) return {tile, std::nullopt};

  // Our tile is a material. We need to get the material index.
  std::size_t mn = this->geom_to_mat_indx(geo_indx);

  return {tile, mn};
}

const std::shared_ptr<DiffusionCrossSection>& DiffusionGeometry::mat(
    std::size_t m) const {
  if (m >= nmats()) {
    auto mssg = "Material index out of range.";
    spdlog::error(mssg);
    throw ScarabeeException(mssg);
  }

  const auto geo_indx = geom_indx(m);
  const auto tile_indx = geom_to_tile_indx(geo_indx);
  return tiles_.element(tile_indx.begin(), tile_indx.end()).xs;
}

xt::svector<std::size_t> DiffusionGeometry::geom_indx(std::size_t m) const {
  if (m >= nmats()) {
    auto mssg = "Material index out of range.";
    spdlog::error(mssg);
    throw ScarabeeException(mssg);
  }

  std::size_t flat_geom_indx = mat_indx_to_flat_geom_indx_[m];
  std::array<std::size_t, 1> flat_geom_vec{flat_geom_indx};

  // Now get the complete geometry index
  auto geom_inds = xt::unravel_indices(flat_geom_vec, geom_shape_,
                                       xt::layout_type::column_major)[0];
  return xt::svector<std::size_t>(geom_inds.begin(), geom_inds.end());
}

xt::svector<std::size_t> DiffusionGeometry::geom_to_tile_indx(
    const xt::svector<std::size_t>& geo_indx) const {
  const std::size_t i = geom_x_indx_to_tile_x_indx(geo_indx[0]);
  if (ndims() == 1) return {i};

  const std::size_t j = geom_y_indx_to_tile_y_indx(geo_indx[1]);
  if (ndims() == 2) return {i, j};

  const std::size_t k = geom_z_indx_to_tile_z_indx(geo_indx[2]);
  return {i, j, k};
}

std::size_t DiffusionGeometry::geom_to_mat_indx(
    const xt::svector<std::size_t>& geo_indx) const {
  // Get the flat geometry index that we can search for
  std::array<xt::svector<std::size_t>, 1> geo_indx_vec{geo_indx};
  const std::size_t geom_flat_indx = xt::ravel_indices(
      geo_indx_vec, geom_shape_, xt::layout_type::column_major)[0];

  const auto it =
      std::lower_bound(mat_indx_to_flat_geom_indx_.begin(),
                       mat_indx_to_flat_geom_indx_.end(), geom_flat_indx);
  return std::distance(mat_indx_to_flat_geom_indx_.begin(), it);

  // Should never get here...
  auto mssg = "Could not find material index.";
  spdlog::error(mssg);
  throw ScarabeeException(mssg);
}

double DiffusionGeometry::volume(std::size_t m) const {
  if (m >= nmats()) {
    auto mssg = "Material index out of range.";
    spdlog::error(mssg);
    throw ScarabeeException(mssg);
  }

  auto indxs = this->geom_indx(m);

  if (ndims() == 1) {
    return dx(indxs[0]);
  } else if (ndims() == 2) {
    return dx(indxs[0]) * dy(indxs[1]);
  } else {
    return dx(indxs[0]) * dy(indxs[1]) * dz(indxs[2]);
  }
}

double DiffusionGeometry::dx(std::size_t i) const {
  const std::size_t t = geom_x_indx_to_tile_x_indx(i);
  return tile_dx_[t] / static_cast<double>(x_divs_per_tile_[t]);
}

double DiffusionGeometry::dy(std::size_t i) const {
  const std::size_t t = geom_y_indx_to_tile_y_indx(i);
  return tile_dy_[t] / static_cast<double>(y_divs_per_tile_[t]);
}

double DiffusionGeometry::dz(std::size_t i) const {
  const std::size_t t = geom_z_indx_to_tile_z_indx(i);
  return tile_dz_[t] / static_cast<double>(z_divs_per_tile_[t]);
}

std::size_t DiffusionGeometry::geom_x_indx_to_tile_x_indx(std::size_t i) const {
  if (i >= nx()) {
    auto mssg = "Index along x is out of range.";
    spdlog::error(mssg);
    throw ScarabeeException(mssg);
  }

  std::size_t i_tile = 0;
  std::size_t l = 0;
  while (l < i) {
    l += x_divs_per_tile_[i_tile];

    if (l > i) break;

    i_tile++;
  }

  return i_tile;
}

std::size_t DiffusionGeometry::geom_y_indx_to_tile_y_indx(std::size_t i) const {
  if (i >= ny()) {
    auto mssg = "Index along y is out of range.";
    spdlog::error(mssg);
    throw ScarabeeException(mssg);
  }

  std::size_t i_tile = 0;
  std::size_t l = 0;
  while (l < i) {
    l += y_divs_per_tile_[i_tile];

    if (l > i) break;

    i_tile++;
  }

  return i_tile;
}

std::size_t DiffusionGeometry::geom_z_indx_to_tile_z_indx(std::size_t i) const {
  if (i >= nz()) {
    auto mssg = "Index along z is out of range.";
    spdlog::error(mssg);
    throw ScarabeeException(mssg);
  }

  std::size_t i_tile = 0;
  std::size_t l = 0;
  while (l < i) {
    l += z_divs_per_tile_[i_tile];

    if (l > i) break;

    i_tile++;
  }

  return i_tile;
}

}  // namespace scarabee