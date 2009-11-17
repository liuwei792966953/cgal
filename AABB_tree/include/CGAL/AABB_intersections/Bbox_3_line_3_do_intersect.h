// Copyright (c) 2008  INRIA Sophia-Antipolis (France), ETH Zurich (Switzerland).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org); you may redistribute it under
// the terms of the Q Public License version 1.0.
// See the file LICENSE.QPL distributed with CGAL.
//
// Licensees holding a valid commercial license may use this file in
// accordance with the commercial license agreement provided with the software.
//
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// $URL$
// $Id$
// 
//
// Author(s)     : Camille Wormser, Jane Tournois, Pierre Alliez, Stephane Tayeb


#ifndef CGAL_LINE_3_BBOX_3_DO_INTERSECT_H
#define CGAL_LINE_3_BBOX_3_DO_INTERSECT_H

#include <CGAL/Line_3.h>
#include <CGAL/Bbox_3.h>

// inspired from http://cag.csail.mit.edu/~amy/papers/box-jgt.pdf

CGAL_BEGIN_NAMESPACE

namespace internal {

  template <typename FT>
  inline
  bool 
  bbox_line_do_intersect_aux(const FT& px, const FT& py, const FT& pz,
                             const FT& qx, const FT& qy, const FT& qz,
                             const FT& bxmin, const FT& bymin, const FT& bzmin,
                             const FT& bxmax, const FT& bymax, const FT& bzmax)
  {
    // -----------------------------------
    // treat x coord
    // -----------------------------------
    FT dmin (0);
    FT tmin (0);
    FT tmax (0);
    if ( qx >= px )
    {
      tmin = bxmin - px;
      tmax = bxmax - px;
      dmin = qx - px;
    }
    else 
    {
      tmin = px - bxmax;
      tmax = px - bxmin;
      dmin = px - qx;
    }

    if ( dmin == FT(0) && (tmin > FT(0) || tmax < FT(0)) )
    {
      return false;
    }
    FT dmax = dmin;
 

    // -----------------------------------
    // treat y coord
    // -----------------------------------
    FT d_ (0);
    FT tmin_ (0);
    FT tmax_ (0);
    if ( qy >= py )
    {
      tmin_ = bymin - py;
      tmax_ = bymax - py;
      d_ = qy - py;
    }
    else
    {
      tmin_ = py - bymax;
      tmax_ = py - bymin;
      d_ = py - qy;
    }
    
    if ( (dmin*tmax_) < (d_*tmin) || (dmax*tmin_) > (d_*tmax) )
      return false;
    
    if( (dmin*tmin_) > (d_*tmin) )
    { 
      tmin = tmin_;
      dmin = d_;
    }
    
    if( (dmax*tmax_) < (d_*tmax) )
    { 
      tmax = tmax_;
      dmax = d_;
    }
    
    
    // -----------------------------------
    // treat z coord
    // -----------------------------------
    if ( qz >= pz )
    {
      tmin_ = bzmin - pz;
      tmax_ = bzmax - pz;
      d_ = qz - pz;
    }
    else
    {
      tmin_ = pz - bzmax;
      tmax_ = pz - bzmin;
      d_ = pz - qz;
    }
    
    return ( (dmin*tmax_) >= (d_*tmin) && (dmax*tmin_) <= (d_*tmax) );
  }
  
  template <class K>
  bool do_intersect(const typename K::Line_3& line, 
                    const CGAL::Bbox_3& bbox,
                    const K&)
  {
    typedef typename K::FT FT;
    typedef typename K::Point_3 Point_3;
    
    const Point_3 point_a = line.point(0);
    const Point_3 point_b = line.point(1);
    
    return bbox_line_do_intersect_aux(
                         point_a.x(), point_a.y(), point_a.z(), 
                         point_b.x(), point_b.y(), point_b.z(),
                         FT(bbox.xmin()), FT(bbox.ymin()), FT(bbox.zmin()),
                         FT(bbox.xmax()), FT(bbox.ymax()), FT(bbox.zmax()) );
  }

} // namespace internal

template <class K>
bool do_intersect(const CGAL::Line_3<K>& line, 
		  const CGAL::Bbox_3& bbox)
{
  return typename K::Do_intersect_3()(line, bbox);
}

template <class K>
bool do_intersect(const CGAL::Bbox_3& bbox, 
		  const CGAL::Line_3<K>& line)
{
  return typename K::Do_intersect_3()(line, bbox);
}

CGAL_END_NAMESPACE

#endif  // CGAL_LINE_3_BBOX_3_DO_INTERSECT_H


