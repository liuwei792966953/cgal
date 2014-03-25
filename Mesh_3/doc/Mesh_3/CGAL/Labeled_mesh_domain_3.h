namespace CGAL {

/*!
\ingroup PkgMesh_3Domains

\brief The class `Labeled_mesh_domain_3` implements indexed domains.
Labeling function f must take its values into N.
Let p be a Point.
 - f(p)=0 means that p is outside domain.
 - f(p)=a, a!=0 means that p is inside subdomain a.
This class is a model of concept `MeshDomain_3`.

Any boundary facet is labeled <a,b>, a<b, where a and b are the
tags of its incident subdomain.
Thus, a boundary facet of the domain is labeled <0,b>, where b!=0.

This class includes a function that provides, the subdomain index of any
query point. An intersection between a segment and bounding
surfaces is detected when both segment endpoints are associated with different
values of subdomain indices. The intersection is then constructed by bisection.
The bisection stops when the query segment is shorter than a given error bound
`e`. This error bound is given by `e=d`\f$ \times\f$`error_bound` where `d` is the
length of the diagonal of the bounding box (in world coordinates), or the radius of the bounding sphere, and
`error_bound` is the argument passed to the constructor of `Labeled_mesh_domain_3`.


\tparam Labeling_function is the type of the input function.

\tparam BGT is a geometric traits class that provides
the basic operations to implement
intersection tests and intersection computations
through a bisection method. This parameter must be instantiated
with a model of the concept `BisectionGeometricTraits_3`.

\cgalModels `MeshDomain_3`

\sa `CGAL::make_mesh_3()`.

*/
template<class Labeling_function, class BGT>
class Labeled_mesh_domain_3
{
public:

/// \name Creation 
/// @{

/*!
\brief Construction from a labeling function and a Sphere as bounding space.
\param f the labeling function.
\param bounding_sphere the bounding sphere of the meshable space.
\param error_bound is the relative error bound used to compute intersection points between the implicit surface and query segments. The
bisection is stopped when the length of the intersected segment is less than the product of `bound` by the radius of
`bounding_sphere`.
*/ 
Labeled_mesh_domain_3(const Labeling_function& f,
                       const Sphere_3& bounding_sphere,
                       const FT& error_bound = FT(1e-3));

/*!
\brief Construction from a labeling function and a Bbox_3 as bounding space.
\param f the labeling function.
\param bbox the bounding box of the meshable space.
\param error_bound is the relative error bound used to compute intersection points between the implicit surface and query segments. The
bisection is stopped when the length of the intersected segment is less than the product of `bound` by the diagonal of
`bounding_box`.
*/
Labeled_mesh_domain_3(const Labeling_function& f,
                       const Bbox_3& bbox,
                       const FT& error_bound = FT(1e-3));

/*!
\brief Construction from a function and an Iso_cuboid_3 as bounding space.
\param f the function.
\param bbox the bounding box of the meshable space.
\param error_bound is the relative error bound used to compute intersection points between the implicit surface and query segments. The
bisection is stopped when the length of the intersected segment is less than the product of `bound` by the diagonal of
`bounding_box`.
*/
Labeled_mesh_domain_3(const Labeling_function& f,
                       const Iso_cuboid_3& bbox,
                       const FT& error_bound = FT(1e-3));

/// @}

}; /* end Labeled_mesh_domain_3 */
} /* end namespace CGAL */
