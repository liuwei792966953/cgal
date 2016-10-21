#ifndef CGAL_MVC_POST_PROCESSOR_3_H
#define CGAL_MVC_POST_PROCESSOR_3_H

#include <CGAL/Two_vertices_parameterizer_3.h>
#include <CGAL/parameterize.h>

#include <CGAL/Eigen_solver_traits.h>
#include <CGAL/Constrained_triangulation_2.h>
#include <CGAL/Constrained_Delaunay_triangulation_2.h>
#include <CGAL/Polygon_mesh_processing/border.h>
#include <CGAL/Triangulation_vertex_base_with_info_2.h>
#include <CGAL/Triangulation_face_base_with_info_2.h>

#include <boost/unordered_set.hpp>
#include <boost/graph/graph_traits.hpp>

#include <vector>
#include <fstream>
#include <iostream>

/// \file CGAL/MVC_post_processor_3.h

// @todo Determine the proper name of this file
// @todo Handle non-simple boundary
// @todo Make it work with a surface mesh type

namespace CGAL {

/// Copy the data from two vectors to the UVmap.
template <typename TriangleMesh,
          typename Vector,
          typename Vertex_set,
          typename VertexUVMap,
          typename VertexIndexMap>
void assign_solution(const Vector& Xu,
                     const Vector& Xv,
                     const Vertex_set& vertices,
                     VertexUVMap uvmap,
                     VertexIndexMap vimap)
{
  typedef Parameterizer_traits_3<TriangleMesh>          Traits;
  typedef typename Traits::NT                           NT;
  typedef typename Traits::Point_2                      Point_2;

  typedef typename boost::graph_traits<TriangleMesh>::vertex_descriptor    vertex_descriptor;

  BOOST_FOREACH(vertex_descriptor vd, vertices){
    int index = get(vimap, vd);
    NT u = Xu(index);
    NT v = Xv(index);

//      Point_2 p = get(uvmap, vd);
//      std::cout << "old: " << p << " || new: " << u << " " << v << std::endl;

    put(uvmap, vd, Point_2(u, v));
  }
}

// ------------------------------------------------------------------------------------
// Declaration
// ------------------------------------------------------------------------------------

template
<
  class TriangleMesh, ///< a model of `FaceGraph`
  class SparseLinearAlgebraTraits_d ///< Traits class to solve a sparse linear system.
    = Eigen_solver_traits<Eigen::BiCGSTAB<Eigen_sparse_matrix<double>::EigenType,
                                          Eigen::IncompleteLUT< double > > >
>
class MVC_post_processor_3
  : public Parameterizer_traits_3<TriangleMesh>
{
// Private types
private:
  // This class
  typedef MVC_post_processor_3<TriangleMesh, SparseLinearAlgebraTraits_d>  Self;

  // Superclass
  typedef Parameterizer_traits_3<TriangleMesh>                             Base;

// Public types
public:
  // We have to repeat the types exported by superclass
  /// @cond SKIP_IN_MANUAL
  typedef typename Base::Error_code                   Error_code;
  /// @endcond


// Private types
private:
  typedef typename boost::graph_traits<TriangleMesh>::vertex_descriptor    vertex_descriptor;
  typedef typename boost::graph_traits<TriangleMesh>::halfedge_descriptor  halfedge_descriptor;
  typedef typename boost::graph_traits<TriangleMesh>::face_descriptor      face_descriptor;
  typedef typename boost::graph_traits<TriangleMesh>::face_iterator        face_iterator;
  typedef typename boost::graph_traits<TriangleMesh>::vertex_iterator      vertex_iterator;

  typedef boost::unordered_set<vertex_descriptor>       Vertex_set;
  typedef std::vector<face_descriptor>                  Faces_vector;

  // Mesh_Adaptor_3 subtypes:
  typedef Parameterizer_traits_3<TriangleMesh>  Traits;
  typedef typename Traits::NT                   NT;
  typedef typename Traits::Point_2              Point_2;
  typedef typename Traits::Point_3              Point_3;
  typedef typename Traits::Vector_2             Vector_2;
  typedef typename Traits::Vector_3             Vector_3;

  // SparseLinearAlgebraTraits_d subtypes:
  typedef SparseLinearAlgebraTraits_d                  Sparse_LA;

  typedef typename Sparse_LA::Vector                   Vector;
  typedef typename Sparse_LA::Matrix                   Matrix;

// Private fields
private:
  /// Traits object to solve a sparse linear system
  Sparse_LA m_linearAlgebra;

// Private accessors
private:
  /// Get the sparse linear algebra (traits object to access the linear system).
  Sparse_LA& get_linear_algebra_traits() { return m_linearAlgebra; }

// Private utility
  /// Print the exterior faces of the constrained triangulation.
  template <typename CT>
  void output_ct_exterior_faces(const CT& ct) const
  {
    std::ofstream out("constrained_triangulation_exterior.txt");
    typename CT::Finite_faces_iterator fit = ct.finite_faces_begin(),
                                       fend = ct.finite_faces_end();
    for(; fit!=fend; ++fit){
      if(fit->info() != -1) // filter interior faces
        continue;

      out << "4 ";
      for(std::size_t i=0; i<4; ++i){
        out << fit->vertex(i%3)->point() << " 0 ";
      }
      out << '\n';
    }
    out << std::endl;
  }

// Private operations
private:
  /// Checks whether the border polygon is simple.
  template <typename VertexUVMap>
  bool is_polygon_simple(const TriangleMesh& mesh,
                         halfedge_descriptor bhd,
                         const VertexUVMap uvmap) const
  {
    typedef typename Traits::Kernel                 Kernel;
    typedef typename Kernel::Segment_2              Segment_2;

    // @fixme unefficient: brute force use sweep line algorithms instead
    // @todo handle the multiple borders

    BOOST_FOREACH(halfedge_descriptor hd_1, halfedges_around_face(bhd, mesh)){
      BOOST_FOREACH(halfedge_descriptor hd_2, halfedges_around_face(bhd, mesh)){
        if(hd_1 == hd_2 || // equality
           next(hd_1, mesh) == hd_2 || next(hd_2, mesh) == hd_1) // adjacency
          continue;

        if(CGAL::do_intersect(Segment_2(get(uvmap, source(hd_1, mesh)),
                                        get(uvmap, target(hd_1, mesh))),
                              Segment_2(get(uvmap, source(hd_2, mesh)),
                                        get(uvmap, target(hd_2, mesh))))){
          std::ofstream out("non-simple.txt");
          out << "2 " << get(uvmap, source(hd_1, mesh)) << " 0 "
                      << get(uvmap, target(hd_1, mesh)) << " 0" << std::endl;
          out << "2 " << get(uvmap, source(hd_2, mesh)) << " 0 "
                      << get(uvmap, target(hd_2, mesh)) << " 0" << std::endl;
          return false;
        }
      }
    }
    return true;
  }

  /// Spread the inside / outside coloring from a Face to its neighbors
  /// depending on whether the common edge is constrained.
  template <typename CT>
  void spread(CT& ct,
              const typename CT::Face_handle fh) const
  {
    typedef typename CT::Edge           Edge;
    typedef typename CT::Face_handle    Face_handle;

    int fh_color = fh->info();
    CGAL_precondition(fh_color); // fh must be colored

    // look at the three neighbors
    for(std::size_t i=0; i<3; ++i)
    {
      // ignore infinite faces and faces that already have a color
      Face_handle neigh_fh = fh->neighbor(i);

      if(ct.is_infinite(neigh_fh)) // infinite
        continue;

      if(neigh_fh->info() != 0) // already colored
        continue;

      bool is_common_edge_constrained = ct.is_constrained(Edge(fh, i));

      // if the edge is constrained, switch the color
      neigh_fh->info() = is_common_edge_constrained ? - fh_color : fh_color;

      spread(ct, neigh_fh);
    }
  }

  /// Triangulate the holes in the convex hull of the parameterization.
  template <typename CT,
            typename VertexUVMap>
  Error_code triangulate_convex_hull(const TriangleMesh& mesh,
                                     const VertexUVMap uvmap,
                                     CT& ct) const
  {
    // Gather border edges
    std::vector<halfedge_descriptor> border_hds;
    CGAL::Polygon_mesh_processing::border_halfedges(mesh,
                                                    std::back_inserter(border_hds));

    // Build the constrained triangulation

    // Since the border is closed and we are interest in triangles that are outside
    // of the border, we actually only need to insert points on the border

    // Insert points
    typename std::vector<halfedge_descriptor>::iterator vhd_iter = border_hds.begin();
    const typename std::vector<halfedge_descriptor>::iterator vhd_end = border_hds.end();
    for(; vhd_iter!=vhd_end; ++vhd_iter){
      halfedge_descriptor hd = *vhd_iter;
      vertex_descriptor s = source(hd, mesh), t = target(hd, mesh);
      Point_2 sp = get(uvmap, s);

      typename CT::Vertex_handle vh = ct.insert(sp);
      vh->info() = s;
//      std::cout << "Added point: " << sp << '\n';
    }

    // Insert constraints
    vhd_iter = border_hds.begin();
    for(; vhd_iter!=vhd_end; ++vhd_iter){
      halfedge_descriptor hd = *vhd_iter;
      vertex_descriptor s = source(hd, mesh), t = target(hd, mesh);
      Point_2 sp = get(uvmap, s), tp = get(uvmap, t);

      ct.insert_constraint(sp, tp);
//      std::cout << "Constrained edge: " << sp << " " << tp << std::endl;
    }

    std::ofstream out("constrained_triangulation.cgal");
    out << ct;

    return Base::OK;
  }

  /// Color the (finite) faces of the constrained triangulation with an inside
  /// or outside tag.
  template <typename CT>
  Error_code color_faces(CT& ct) const
  {
    typedef typename CT::Edge                               Edge;
    typedef typename CT::Face_handle                        Face_handle;

    // Initialize all colors to 0.
    // 'Inside' is the color '1' and 'outside' is '-1'
    typename CT::Finite_faces_iterator fit = ct.finite_faces_begin(),
                                       fend = ct.finite_faces_end();
    for(; fit!=fend; ++fit)
      fit->info() = 0;

    // start from an infinite face
    Face_handle infinite_face = ct.infinite_face();
    int index_of_inf_v = infinite_face->index(ct.infinite_vertex());
    int mirror_index = infinite_face->mirror_index(index_of_inf_v);

    Face_handle start_face = infinite_face->neighbor(index_of_inf_v);
    CGAL_assertion(!ct.is_infinite(start_face));

    bool is_edge_constrained = ct.is_constrained(Edge(start_face, mirror_index));

    // If the edge is constrained, 'start_face' is an interior face, otherwise
    // it is an exterior face
    int color = is_edge_constrained ? 1 : -1;
    start_face->info() = color;

    spread<CT>(ct, start_face);

    // Check that everything went smoothly
    CGAL_expensive_postcondition_code(
      // Look at all the finite faces and make sure that:
      // -- a constrained edge has faces of different colors on each side
      // -- an unconstrained edge has faces of same colors on each side

      fit = ct.finite_faces_begin();
      for(; fit!=fend; ++fit)
      {
        Face_handle fh_1 = fit;
        CGAL_assertion(fh_1->info() != 0);

        for(std::size_t i=0; i<3; ++i){
          Face_handle fh_2 = fh_1->neighbor(i);

          if(ct.is_infinite(fh_2))
            continue;

          CGAL_assertion(fh_2->info() != 0);

          bool is_common_edge_constrained = ct.is_constrained(Edge(fh_1, i));
          if(is_common_edge_constrained){
            CGAL_assertion(fh_1->info() == -1 * fh_2->info());
          }
          else
            CGAL_assertion(fh_1->info() == fh_2->info());
        }
      }
      std::cout << "Passed the color check" << std::endl;
    );

    // Output the exterior faces of the constrained triangulation
    output_ct_exterior_faces(ct);

    return Base::OK;
  }

  //                                                       -> ->
  /// Return angle (in radians) of of (P,Q,R) corner (i.e. QP,QR angle).
  double compute_angle_rad(const Point_2& P,
                           const Point_2& Q,
                           const Point_2& R) const
  {
    Vector_2 u = P - Q;
    Vector_2 v = R - Q;

    double angle = std::atan2(v.y(), v.x()) - std::atan2(u.y(), u.x());
    if(angle < 0)
      angle += 2 * CGAL_PI;

    return angle;
  }

  /// Fix vertices that are on the convex hull ahead of the MVC parameterization.
  template <typename CT,
            typename VertexParameterizedMap>
  void fix_convex_hull_border(const CT& ct,
                              VertexParameterizedMap vpmap) const
  {
    typename CT::Vertex_circulator vc = ct.incident_vertices(ct.infinite_vertex()),
                                   vend = vc;
    do{
      vertex_descriptor vd = vc->info();
      put(vpmap, vd, true);
    } while (++vc != vend);
  }

  NT compute_w_ij_mvc(const Point_2& pi, const Point_2& pj, const Point_2& pk) const
  {
    //                                                               ->     ->
    // Compute the angle (pj, pi, pk), the angle between the vectors ij and ik
    NT angle = compute_angle_rad(pj, pi, pk);

    // For flipped triangles, the connectivity is inversed and thus the angle
    // computed by the previous function is not the one we need. Instead,
    // we need the explementary angle.
    if(angle > CGAL_PI){ // flipped triangle
      angle = 2 * CGAL_PI - angle;
    }
    NT weight = std::tan(0.5 * angle);

    return weight;
  }

  void fill_linear_system_matrix_mvc_from_points(const Point_2& pi, int i,
                                                 const Point_2& pj, int j,
                                                 const Point_2& pk, int k,
                                                 Matrix& A) const
  {
    // For MVC, the entry of A(i,j) is - [ tan(gamma_ij/2) + tan(delta_ij)/2 ] / |ij|
    // where gamma_ij and delta_ij are the angles at i around the edge ij

    // This function computes the angle alpha at i, and add
    // -- A(i,j) += tan(alpha / 2) / |ij|
    // -- A(i,k) += tan(alpha / 2) / |ik|
    // -- A(i,i) -= A(i,j) + A(i,k)

    // The other parts of A(i,j) and A(i,k) will be added when this function
    // is called from the neighboring faces of F_ijk that share the vertex i

    // Compute: - tan(alpha / 2)
    NT w_i_base = -1.0 * compute_w_ij_mvc(pi, pj, pk);

    // @fixme unefficient: lengths are computed (and inversed!) twice per edge

    // Set w_ij in matrix
    Vector_2 edge_ij = pi - pj;
    double len_ij = std::sqrt(edge_ij * edge_ij);
    CGAL_assertion(len_ij != 0.0); // two points are identical!
    NT w_ij = w_i_base / len_ij;
    A.add_coef(i, j, w_ij);

    // Set w_ik in matrix
    Vector_2 edge_ik = pi - pk;
    double len_ik = std::sqrt(edge_ik * edge_ik);
    CGAL_assertion(len_ik != 0.0); // two points are identical!
    NT w_ik = w_i_base / len_ik;
    A.add_coef(i, k, w_ik);

    // Add to w_ii (w_ii = - sum w_ij)
    NT w_ii = - w_ij - w_ik;
    A.add_coef(i, i, w_ii);
  }

  /// Partially fill the matrix coefficients A(i,i), A(i,j) and A(i,k)
  /// Precondition: i, j, and k are ordered counterclockwise
  template <typename CT,
            typename VertexUVMap,
            typename VertexIndexMap,
            typename VertexParameterizedMap>
  void fill_linear_system_matrix_mvc_from_ct_vertices(typename CT::Vertex_handle vh_i,
                                                      typename CT::Vertex_handle vh_j,
                                                      typename CT::Vertex_handle vh_k,
                                                      const VertexUVMap uvmap,
                                                      const VertexIndexMap vimap,
                                                      const VertexParameterizedMap vpmap,
                                                      Matrix& A) const
  {
    vertex_descriptor vd_i = vh_i->info();
    vertex_descriptor vd_j = vh_j->info();
    vertex_descriptor vd_k = vh_k->info();

    // Coordinates are grabbed from the uvmap
    const Point_2& pi = get(uvmap, vd_i);
    const Point_2& pj = get(uvmap, vd_j);
    const Point_2& pk = get(uvmap, vd_k);
    int i = get(vimap, vd_i);
    int j = get(vimap, vd_j);
    int k = get(vimap, vd_k);

    // if vh_i is fixed, there is nothing to do: A(i,i)=1 and A(i,j)=0 for j!=i
    if(get(vpmap, vd_i))
    {
//      std::cout << i << " is fixed in A " << std::endl;
      // @fixme unefficient: A(i,i) is written as many times as i has neighbors
      A.set_coef(i, i, 1);
      return;
    }

    // vh_i is NOT fixed
    fill_linear_system_matrix_mvc_from_points(pi, i, pj, j, pk, k, A);
  }

  /// Add the corresponding coefficients in A for all the edges of the face fh
  template <typename CT,
            typename VertexUVMap,
            typename VertexIndexMap,
            typename VertexParameterizedMap>
  void fill_linear_system_matrix_mvc_from_ct_face(const CT& ct,
                                                  typename CT::Finite_faces_iterator fh,
                                                  const VertexUVMap uvmap,
                                                  const VertexIndexMap vimap,
                                                  const VertexParameterizedMap vpmap,
                                                  Matrix& A) const
  {
    CGAL_precondition(!ct.is_infinite(fh));
    typedef typename CT::Vertex_handle                    Vertex_handle;

    // Doing it explicitely rather than a loop for clarity
    Vertex_handle vh0 = fh->vertex(0);
    Vertex_handle vh1 = fh->vertex(1);
    Vertex_handle vh2 = fh->vertex(2);

    fill_linear_system_matrix_mvc_from_ct_vertices<CT>(vh0, vh1, vh2,
                                                       uvmap, vimap, vpmap, A);
    fill_linear_system_matrix_mvc_from_ct_vertices<CT>(vh1, vh2, vh0,
                                                       uvmap, vimap, vpmap, A);
    fill_linear_system_matrix_mvc_from_ct_vertices<CT>(vh2, vh0, vh1,
                                                       uvmap, vimap, vpmap, A);
  }

  template <typename VertexUVMap,
            typename VertexIndexMap,
            typename VertexParameterizedMap>
  void fill_linear_system_matrix_mvc_from_mesh_halfedge(const TriangleMesh& mesh,
                                                        halfedge_descriptor hd,
                                                        const VertexUVMap uvmap,
                                                        const VertexIndexMap vimap,
                                                        const VertexParameterizedMap vpmap,
                                                        Matrix& A) const
  {
    vertex_descriptor vd_i = target(hd, mesh);
    vertex_descriptor vd_j = target(next(hd, mesh), mesh);
    vertex_descriptor vd_k = source(hd, mesh);
    const Point_2& pi = get(uvmap, vd_i);
    const Point_2& pj = get(uvmap, vd_j);
    const Point_2& pk = get(uvmap, vd_k);
    int i = get(vimap, vd_i);
    int j = get(vimap, vd_j);
    int k = get(vimap, vd_k);

    // if vh_i is fixed, there is nothing to do: A(i,i)=1 and A(i,j)=0 for j!=i
    if(get(vpmap, vd_i))
    {
      std::cout << i << " is fixed in A " << std::endl;
      // @fixme unefficient A(i,i) is written as many times as i has neighbors
      A.set_coef(i, i, 1);
      return;
    }

    // Below, vh_i is NOT fixed
    fill_linear_system_matrix_mvc_from_points(pi, i, pj, j, pk, k, A);
  }

  /// Fill the matrix A in an MVC linear system with the face 'fd' of 'mesh'.
  template <typename VertexUVMap,
            typename VertexIndexMap,
            typename VertexParameterizedMap>
  void fill_linear_system_matrix_mvc_from_mesh_face(const TriangleMesh& mesh,
                                                    face_descriptor fd,
                                                    const VertexUVMap uvmap,
                                                    const VertexIndexMap vimap,
                                                    const VertexParameterizedMap vpmap,
                                                    Matrix& A) const
  {
    halfedge_descriptor hd = halfedge(fd, mesh);

    fill_linear_system_matrix_mvc_from_mesh_halfedge(mesh, hd, uvmap, vimap, vpmap, A);
    fill_linear_system_matrix_mvc_from_mesh_halfedge(mesh, next(hd, mesh),
                                                     uvmap, vimap, vpmap, A);
    fill_linear_system_matrix_mvc_from_mesh_halfedge(mesh, prev(hd, mesh),
                                                     uvmap, vimap, vpmap, A);
  }

  /// Compute the matrix A in the MVC linear system using the exterior faces
  /// of the constrained triangulation 'ct' and the graph 'mesh'.
  template <typename CT,
            typename VertexUVMap,
            typename VertexIndexMap,
            typename VertexParameterizedMap>
  Error_code compute_mvc_matrix(const CT& ct,
                                const TriangleMesh& mesh,
                                const Faces_vector& faces,
                                const VertexUVMap uvmap,
                                const VertexIndexMap vimap,
                                const VertexParameterizedMap vpmap,
                                Matrix& A) const
  {
    Error_code status = Base::OK;

    // The constrained triangulation has only "real" faces in the holes of
    // the convex hull of 'mesh'.

    // Thus, coefficients will come from 'ct' for faces that are in the convex hull
    // but not in 'mesh' (those were colored '-1' in color_faces() ) and
    // from 'mesh' for faces that are in the convex hull but not in 'ct'

    // Loop over the "outside" faces of ct
    std::cout << "add from ct" << std::endl;
    typename CT::Finite_faces_iterator fit = ct.finite_faces_begin(),
                                       fend = ct.finite_faces_end();
    for(; fit!=fend; ++fit)
    {
      CGAL_precondition(fit->info() != 0);
      if(fit->info() == 1) // "interior" face
        continue;

      CGAL_precondition(fit->info() == -1);
      fill_linear_system_matrix_mvc_from_ct_face(ct, fit, uvmap, vimap, vpmap, A);
    }

    // Loop over the faces of 'mesh'
    std::cout << "add from mesh" << std::endl;
    BOOST_FOREACH(face_descriptor fd, faces){
      fill_linear_system_matrix_mvc_from_mesh_face(mesh, fd, uvmap, vimap, vpmap, A);
    }

    return status;
  }

  /// Compute the right hand side of a MVC linear system.
  template <typename VertexUVMap,
            typename VertexIndexMap,
            typename VertexParameterizedMap>
  void compute_mvc_rhs(const Vertex_set& vertices,
                       const VertexUVMap uvmap,
                       const VertexIndexMap vimap,
                       const VertexParameterizedMap vpmap,
                       Vector& Bu, Vector& Bv) const
  {
    BOOST_FOREACH(vertex_descriptor vd, vertices){
      int index = get(vimap, vd);
      Point_2 uv = get(uvmap, vd);
      if(!get(vpmap, vd)){ // not yet parameterized
        Bu[index] = 0.; // might not be needed
        Bv[index] = 0.;
      } else { // fixed vertices
        Bu[index] = uv.x();
        Bv[index] = uv.y();
      }
    }
  }

  /// Solve the linear systems A*Xu=Bu and A*Xv=Bv using Eigen's BiCGSTAB
  /// and incompleteLUT factorization.
  Error_code solve_mvc(const Matrix& A,
                       const Vector& Bu, const Vector& Bv,
                       Vector& Xu, Vector& Xv) const
  {
    Error_code status = Base::OK;

    typedef Eigen_solver_traits<
              Eigen::BiCGSTAB<Eigen_sparse_matrix<double>::EigenType,
                              Eigen::IncompleteLUT<double> > >      EigenSolver;
    EigenSolver solver;

    NT Du, Dv;
    if(!solver.linear_solver(A, Bu, Xu, Du) ||
       !solver.linear_solver(A, Bv, Xv, Dv)){
      status = Base::ERROR_CANNOT_SOLVE_LINEAR_SYSTEM;
    }

    return status;
  }

  template <typename CT, typename VertexParameterizedMap>
  Error_code prepare_CT_for_parameterization(const CT& ct,
                                             VertexParameterizedMap mvc_vpmap) const
  {
    Error_code status = Base::OK;

    // Gather the finite faces of the CT that are not faces of 'mesh'
    status = color_faces(ct);
    if(status != Base::OK)
      return status;

    // Gather the vertices that are on the border of the convex hull and will be fixed
    fix_convex_hull_border(ct, mvc_vpmap);

    return status;
  }

  /// Run an MVC parameterization on the (2D) ARAP UV map and the convexified mesh.
  template <typename CT,
            typename VertexUVMap,
            typename VertexIndexMap,
            typename VertexParameterizedMap>
  Error_code parameterize_convex_hull_with_MVC(const TriangleMesh& mesh,
                                               const Vertex_set& vertices,
                                               const Faces_vector& faces,
                                               const CT& ct,
                                               VertexUVMap uvmap,
                                               const VertexIndexMap vimap,
                                               const VertexParameterizedMap mvc_vpmap) const
  {
    Error_code status = Base::OK;

    // Matrices and vectors needed in the resolution
    int nbVertices = num_vertices(mesh);
    Matrix A(nbVertices, nbVertices);
    Vector Xu(nbVertices), Xv(nbVertices), Bu(nbVertices), Bv(nbVertices);

    // Compute the (constant) matrix A
    compute_mvc_matrix(ct, mesh, faces, uvmap, vimap, mvc_vpmap, A);

    // Compute the right hand side of the linear system
    compute_mvc_rhs(vertices, uvmap, vimap, mvc_vpmap, Bu, Bv);

    // Solve the linear system
    status = solve_mvc(A, Bu, Bv, Xu, Xv);

    // Assign the UV values
    assign_solution<TriangleMesh>(Xu, Xv, vertices, uvmap, vimap);

    return Base::OK;
  }

public:
  /// Use the convex virtual boundary algorithm of Karni et al.[2005] to fix
  /// the (hopefully few) flips in the result.
  template <typename VertexUVMap,
            typename VertexIndexMap>
  Error_code parameterize(const TriangleMesh& mesh,
                          const Vertex_set& vertices,
                          const Faces_vector& faces,
                          halfedge_descriptor bhd,
                          VertexUVMap uvmap,
                          const VertexIndexMap vimap) const
  {
    typedef typename Traits::Kernel                                        Kernel;

    // Each triangulation vertex is associated its corresponding vertex_descriptor
    typedef CGAL::Triangulation_vertex_base_with_info_2<vertex_descriptor,
                                                        Kernel>             Vb;
    // Each triangultaion face is associated a color (inside/outside information)
    typedef CGAL::Triangulation_face_base_with_info_2<int, Kernel>          Fb;
    typedef CGAL::Constrained_triangulation_face_base_2<Kernel, Fb>         Cfb;
    typedef CGAL::Triangulation_data_structure_2<Vb, Cfb>                   TDS;
    typedef CGAL::No_intersection_tag                                       Itag;
    typedef CGAL::Constrained_triangulation_2<Kernel, TDS, Itag>            CT;
//    typedef CGAL::Constrained_Delaunay_triangulation_2<Kernel, TDS, Itag>   CT;

    // Check if the polygon is simple
    const bool is_param_border_simple = is_polygon_simple(mesh, bhd, uvmap);

    // not sure how to handle non-simple yet
    CGAL_postcondition(is_param_border_simple);
    std::cout << "Border is simple!" << std::endl;

    // Triangulate the holes in the convex hull of the polygon
    CT ct;
    triangulate_convex_hull<CT>(mesh, uvmap, ct);

    // Prepare the constrained triangulation by coloring faces and fixing the border
    boost::unordered_set<vertex_descriptor> vs;
    CGAL::internal::Bool_property_map<boost::unordered_set<vertex_descriptor> > mvc_vpmap(vs);
    prepare_CT_for_parameterization(ct, mvc_vpmap);

    // Run the MVC
    parameterize_convex_hull_with_MVC(mesh, vertices, faces, ct, uvmap, vimap, mvc_vpmap);

    return Base::OK;
  }
};

} // namespace CGAL

#endif // CGAL_MVC_POST_PROCESSOR_3_H
