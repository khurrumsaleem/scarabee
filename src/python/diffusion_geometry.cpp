#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <diffusion/diffusion_geometry.hpp>

namespace py = pybind11;

using namespace scarabee;

void init_DiffusionGeometry(py::module& m) {
  // Tile definition
  py::class_<DiffusionGeometry::Tile>(
      m, "DiffusionGeometryTile",
      "A DiffusionGeometryTile represents an element of a cartesian diffusion "
      "mesh. It can have either an albedo entry (float) or a xs entry "
      "(:py:class:`DiffusionCrossSection`), but not both.")

      .def_readwrite("albedo", &DiffusionGeometry::Tile::albedo,
                     "The albedo if the tile is a boundary condition.")

      .def_readwrite(
          "xs", &DiffusionGeometry::Tile::xs,
          "The DiffusionCrossSection if the tile represents a material.");

  py::enum_<DiffusionGeometry::Neighbor>(m, "Neighbor")
      .value("XN", DiffusionGeometry::Neighbor::XN)
      .value("XP", DiffusionGeometry::Neighbor::XP)
      .value("YN", DiffusionGeometry::Neighbor::YN)
      .value("YP", DiffusionGeometry::Neighbor::YP)
      .value("ZN", DiffusionGeometry::Neighbor::ZN)
      .value("ZP", DiffusionGeometry::Neighbor::ZP);

  py::class_<DiffusionGeometry, std::shared_ptr<DiffusionGeometry>>(
      m, "DiffusionGeometry",
      "A DiffusionGeometry represents a cartesian mesh used to solve diffusion "
      "problems.")

      .def(py::init<const std::vector<double>& /*dx*/, double /*albedo_xn*/,
                    double /*albedo_xp*/>(),
           "Creates a 1D DiffusionGeometry.\n\n"
           "Parameters\n"
           "----------\n"
           "dx : list of float\n"
           "     Width of each mesh.\n"
           "albedo_xn : float\n"
           "            Albedo at the negative x boundary.\n"
           "albedo_xp : float\n"
           "            Albedo at the positive x boundary.\n\n")

      .def("set_tiles", &DiffusionGeometry::set_tiles,
           "Initializes all of the tiles in the geometry with a list of tiles. "
           "Each "
           "entry can either be a float (indicating albedo boundary "
           "condition), or a "
           ":py:class:`DiffusionCrossSection` (indicating a material). A 1D "
           "problem "
           "may not have any albedo tiles. All albedos must be in the range "
           "[0,1].\n\n"
           "Parameters\n"
           "----------\n"
           "tiles : list of float or DiffusionCrossSection\n"
           "        All tiles in the geometry.",
           py::arg("tiles"))

      .def(
          "neighbor", &DiffusionGeometry::neighbor,
          "Obtains the desired neighboring DiffusionGeometryTile and index for "
          "material m. If the neighbor does not exist, an albedo tile is "
          "returned "
          "and the neighbor index is None.\n\n"
          "Parameters\n"
          "----------\n"
          "m : int\n"
          "    Material index.\n"
          "n : Neighbor\n"
          "    Desired neighbor of tile m.\n\n"
          "Returns\n"
          "-------\n"
          "DiffusionGeometryTile\n"
          "                     Tile of the desired neighbor.\n"
          "int (optional)\n"
          "              The material index of the neighbor (if neighbor is a "
          "material).\n",
          py::arg("m"), py::arg("n"))

      .def("mat", &DiffusionGeometry::mat,
           "Obtains the :py:class:`DiffusionCrossSection` for material m.\n\n"
           "Parameters\n"
           "----------\n"
           "m : int\n"
           "    Material index.\n\n"
           "Returns\n"
           "-------\n"
           "DiffusionCrossSection\n"
           "                     Cross section data for material m.\n",
           py::arg("m"))

      .def("mat_indxs", &DiffusionGeometry::mat_indxs,
           "The geometry indices for material index m.\n\n"
           "Parameters\n"
           "----------\n"
           "m : int\n"
           "    Material index.\n\n"
           "Returns\n"
           "-------\n"
           "list of int\n"
           "           Geometry indices of material index m.\n",
           py::arg("m"))

      .def("dx", &DiffusionGeometry::dx,
           "Width in the x direction of the i mesh along the x axis.\n\n"
           "Parameters\n"
           "----------\n"
           "i : int\n"
           "    Mesh index along x-axis.\n\n"
           "Returns\n"
           "float\n"
           "     Width of mesh along x-axis.\n",
           py::arg("i"))

      .def("dy", &DiffusionGeometry::dy,
           "Width in the y direction of the j mesh along the y axis.\n\n"
           "Parameters\n"
           "----------\n"
           "j : int\n"
           "    Mesh index along y-axis.\n\n"
           "Returns\n"
           "float\n"
           "     Width of mesh along y-axis.\n",
           py::arg("j"))

      .def("dz", &DiffusionGeometry::dz,
           "Width in the z direction of the k mesh along the z axis.\n\n"
           "Parameters\n"
           "----------\n"
           "k : int\n"
           "    Mesh index along z-axis.\n\n"
           "Returns\n"
           "float\n"
           "     Width of mesh along z-axis.\n",
           py::arg("k"))

      .def_property_readonly("ngroups", &DiffusionGeometry::ngroups,
                             "Number of energy groups.")

      .def_property_readonly("ndims", &DiffusionGeometry::ndims,
                             "Number of dimensions (1, 2, or 3).")

      .def_property_readonly(
          "ntiles", &DiffusionGeometry::ntiles,
          "Total number of :py:class:`DiffusionGeometryTile` in the problem. "
          "Includes material and boundary condition tiles.")

      .def_property_readonly("nmats", &DiffusionGeometry::nmats,
                             "Total number of material tiles.")

      .def_property_readonly("nx", &DiffusionGeometry::nx,
                             "Number of tiles along the x-axis.")

      .def_property_readonly("ny", &DiffusionGeometry::ny,
                             "Number of tiles along the y-axis.")

      .def_property_readonly("nz", &DiffusionGeometry::nz,
                             "Number of tiles along the z-axis.");
}