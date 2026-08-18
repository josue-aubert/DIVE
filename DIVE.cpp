/*******************************************************************************
* DIVE.cpp: this is the source code for DIVE.

* DIVE: Delaunay trIangulation Void findEr.

* Github repository:
        https://github.com/cheng-zhao/DIVE

* Copyright (c) 2015--2021 Cheng Zhao <zhaocheng03@gmail.com>

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program. If not, see <https://www.gnu.org/licenses/>.

*******************************************************************************/

#include <CGAL/Exact_predicates_exact_constructions_kernel.h> /* CGAL kernel */
#include <CGAL/Periodic_3_Delaunay_triangulation_traits_3.h>  /* periodic DT */
#include <CGAL/Periodic_3_Delaunay_triangulation_3.h>         /* periodic DT */
#include <CGAL/Delaunay_triangulation_3.h>                    /* regular DT  */

#include <iostream>
#include <stdlib.h>
#include <string>
#include <fstream>
#include <vector>
#include <algorithm>
#include <utility> 

/* Data types for Delaunay Triangulation. */
typedef CGAL::Exact_predicates_exact_constructions_kernel K;
typedef CGAL::Periodic_3_Delaunay_triangulation_traits_3<K>     GT;
typedef CGAL::Periodic_3_Delaunay_triangulation_3<GT>           P3DT3;
typedef CGAL::Delaunay_triangulation_3<K>                       DT3;

typedef P3DT3::Periodic_tetrahedron_iterator    tetra_iterator;
typedef DT3::Finite_cells_iterator              cell_iterator;

/* Basic geometric data types. */
typedef P3DT3::Iso_cuboid       Iso_cuboid;
typedef CGAL::Tetrahedron_3<K>  Tetrahedron;
typedef CGAL::Point_3<K>        Point_3;
typedef CGAL::Vector_3<K>   Vector_3;

/* Basic numerical type. */
typedef K::FT FT;

/* Custom definitions. */
#define P_WRN           std::cerr << "\x1B[35;1mWarning:\x1B[0m "
#define P_ERR           std::cerr << "\x1B[31;1mError:\x1B[0m "
#define DIVE_PRECISION  10


void help(void) {
  std::cout << "Usage: DIVE [OPTION]" << std::endl
      << "DIVE: Delaunay trIangulation Void findEr" << std::endl
      << "  -h    Display this message and exit" << std::endl
      << "  -i    Specify the input catalog" << std::endl
      << "  -o    Specify the output catalog" << std::endl
      << "  -l    Lower boundary of the periodic box (default: 0)" << std::endl
      << "  -u    Upper boundary of the periodic box" << std::endl
      << "        Periodic boundary condition is only enabled if '-u' is set"
      << std::endl
      << "Github repository: https://github.com/josue-aubert/DIVE" << std::endl
      << "License: GPLv3" << std::endl;
}


int main(int argc, char *argv[]) {
  std::string ifile, ofile, Bmin_str, Bmax_str;
  int opt;

  /* Parse command line options. */
  while ((opt = getopt(argc, argv, "hi:o:l:u:")) != EOF) {
    switch (opt) {
      case 'h': help(); return 0;
      case 'i': ifile.assign(optarg); break;
      case 'o': ofile.assign(optarg); break;
      case 'l': Bmin_str.assign(optarg); break;
      case 'u': Bmax_str.assign(optarg); break;
      default: break;
    }
  }
  if (optind < argc) {
    P_WRN << "unknown command line options:" << std::endl;
    while (optind < argc) std::cerr << " " << argv[optind++];
    std::cerr << std::endl;
  }

  /* Verify configurations. */
  if (ifile.empty()) {
    P_ERR << "please set the input catalog using the '-i' option" << std::endl;
    return EXIT_FAILURE;
  }
  if (ofile.empty()) {
    P_ERR << "please set the output catalog using the '-o' option" << std::endl;
    return EXIT_FAILURE;
  }

  FT Bmin, Bmax;
  bool box = !Bmax_str.empty();
  if (box) {
    Bmax = std::stod(Bmax_str);
    Bmin = Bmin_str.empty() ? 0 : std:: stod(Bmin_str);
    if (Bmin >= Bmax) {
      P_ERR << "invalid box boundaries: [" << Bmin << ", " << Bmax << "]"
          << std::endl;
      return EXIT_FAILURE;
    }
  }

  /* Read input file. */
  std::cout << "Reading file: " << ifile << std::endl;
  std::ifstream istr(ifile, std::ios::in);
  std::string line;
  std::vector<Point_3> points;

  while (std::getline(istr, line)) {
    std::istringstream iss(line);
    Point_3 p;
    if (!(iss >> p)) {
      P_ERR << "failed to read coordinates from line: " << std::endl
          << line << std::endl;
      return EXIT_FAILURE;
    }
    points.push_back(p);
  }
  istr.close();

  std::size_t psize = points.size();
  if (psize < 4) {
    P_ERR << "too few objects read from file: " << psize << std::endl;
    return EXIT_FAILURE;
  }
  std::cout << "Number of input objects: " << psize << std::endl << std::endl;

  if (box) {    /* Delaunay triangulation with periodic boundary condition */
    Iso_cuboid domain(Bmin, Bmin, Bmin, Bmax, Bmax, Bmax);
    FT Bsize = Bmax - Bmin;

    std::cout << "Building periodic Delaunay Triangulation ..." << std::endl;
    P3DT3 DT(points.begin(), points.end(), domain);
    if (!DT.is_valid()) {
      P_ERR << "failed to build periodic Delaunay Triangulation" << std::endl;
      return EXIT_FAILURE;
    }
    std::cout << "Number of cells: " << DT.number_of_finite_cells()
        << std::endl << std::endl;

    /* Clean input coordinates. */
    std::vector<Point_3>().swap(points);

    /* Save outputs. */
     std::cout << "Writing to file: " << ofile << std::endl;
     std::ofstream ostr(ofile, std::ios::out);
     ostr.precision(DIVE_PRECISION);

     for (tetra_iterator tetra = DT.periodic_tetrahedra_begin();
          tetra != DT.periodic_tetrahedra_end(); tetra++) {
       Tetrahedron tet = DT.construct_tetrahedron(*tetra);
       Point_3 Pcen = CGAL::circumcenter(tet);
       FT radius = CGAL::squared_distance(Pcen, tet[0]);

       /* Move centers into the box. */
       FT x, y, z;
       x = Pcen[0];
       y = Pcen[1];
       z = Pcen[2];
       if (x < Bmin) x += Bsize;
       else if (x >= Bmax) x -= Bsize;
       if (y < Bmin) y += Bsize;
       else if (y >= Bmax) y -= Bsize;
       if (z < Bmin) z += Bsize;
       else if (z >= Bmax) z -= Bsize;

       /* Compute parity */
       Point_3 r_a = tet[0], r_b = tet[1], r_c = tet[2], r_d = tet[3];

       K::FT l_ab = CGAL::squared_distance(r_a, r_b), l_ac = CGAL::squared_distance(r_a, r_c), 
             l_ad = CGAL::squared_distance(r_a, r_d), l_bc = CGAL::squared_distance(r_b, r_c), 
             l_bd = CGAL::squared_distance(r_b, r_d), l_cd = CGAL::squared_distance(r_c, r_d);
       K::FT s_a = l_ab + l_ac + l_ad, s_b = l_ab + l_bc + l_bd, 
             s_c = l_ac + l_bc + l_cd, s_d = l_ad + l_bd + l_cd;

       std::vector<std::pair<K::FT, Point_3>> v = {{s_a, r_a}, {s_b, r_b}, {s_c, r_c}, {s_d, r_d}};
      
       std::sort(v.begin(), v.end(), [](const std::pair<K::FT,Point_3>& x,
        const std::pair<K::FT,Point_3>& y) {return x.first < y.first;});
       Point_3 r_0 = v[0].second, r_1 = v[1].second, r_2 = v[2].second, r_3 = v[3].second;
      
       Vector_3 s_1 = r_1 - r_0, s_2 = r_2 - r_0, s_3 = r_3 - r_0;
       double chirality = CGAL::to_double(CGAL::determinant(s_1, s_2, s_3));
       double chirality_shape = chirality / (CGAL::sqrt(CGAL::to_double(s_1.squared_length()))
                          * CGAL::sqrt(CGAL::to_double(s_2.squared_length()))
                          * CGAL::sqrt(CGAL::to_double(s_3.squared_length())));  // In [-1, 1]
      
       ostr << x << " " << y << " " << z << " " 
       << CGAL::sqrt(CGAL::to_double(radius)) << " " << chirality << " " << chirality_shape << std::endl;
     }
     ostr.close();
  }
  else {        /* Delaunay triangulation without periodic boundary confition */
    std::cout << "Building Delaunay Triangulation ..." << std::endl;
    DT3 DT(points.begin(), points.end());
    if (!DT.is_valid()) {
      P_ERR << "failed to build Delaunay Triangulation" << std::endl;
      return EXIT_FAILURE;
    }
    std::cout << "Number of cells: " << DT.number_of_finite_cells()
        << std::endl << std::endl;

    /* Clean input coordinates. */
    std::vector<Point_3>().swap(points);

    /* Save outputs. */
    std::cout << "Writing to file: " << ofile << std::endl;
    std::ofstream ostr(ofile, std::ios::out);
    ostr.precision(DIVE_PRECISION);

    for (cell_iterator cell = DT.finite_cells_begin();
         cell != DT.finite_cells_end(); cell++) {
      Point_3 Pcen = cell->circumcenter();
      FT radius = CGAL::squared_distance(Pcen, cell->vertex(0)->point());


      ostr << Pcen << " " << CGAL::sqrt(CGAL::to_double(radius)) << std::endl;
    }
    ostr.close();
  }

  std::cout << "DIVE finished successfully" << std::endl;
  return 0;
}
