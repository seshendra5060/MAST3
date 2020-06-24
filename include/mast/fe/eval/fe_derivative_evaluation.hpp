
#ifndef __mast_fe_derivative_evaluation_h__
#define __mast_fe_derivative_evaluation_h__

// MAST includes
#include <mast/base/mast_data_types.h>
#include <mast/base/exceptions.hpp>


namespace MAST {
namespace FEBasis {
namespace Evaluation {

template <typename NodalScalarType,
          uint_t ElemDim,
          uint_t SpatialDim,
          typename FEBasisType,
          typename ContextType>
inline void
compute_xyz(const ContextType& c,
            const FEBasisType& fe_basis,
            Eigen::Matrix<NodalScalarType, SpatialDim, Eigen::Dynamic>& node_coord,
            Eigen::Matrix<NodalScalarType, SpatialDim, Eigen::Dynamic>& xyz) {
    
    uint_t
    nq      = fe_basis.n_q_points(),
    n_nodes = c.n_nodes();

    node_coord  = Eigen::Matrix<NodalScalarType, SpatialDim, Eigen::Dynamic>::Zero(SpatialDim, n_nodes);
    xyz         = Eigen::Matrix<NodalScalarType, SpatialDim, Eigen::Dynamic>::Zero(SpatialDim, nq);

    // get the nodal locations
    for (uint_t i=0; i<n_nodes; i++)
        for (uint_t j=0; j<SpatialDim; j++)
            node_coord(j, i) = c.nodal_coord(i, j);

    // quadrature point coordinates
    for (uint_t i=0; i<nq; i++)
        for (uint_t k=0; k<n_nodes; k++)
            for (uint_t j=0; j<SpatialDim; j++)
                xyz(j, i) += fe_basis.phi(i, k) * node_coord(j, k);
}



template <typename NodalScalarType,
          uint_t ElemDim,
          uint_t SpatialDim,
          typename FEBasisType,
          typename ContextType>
inline void
compute_Jac(const ContextType& c,
            const FEBasisType& fe_basis,
            const Eigen::Matrix<NodalScalarType, SpatialDim, Eigen::Dynamic>& node_coord,
            Eigen::Matrix<NodalScalarType, SpatialDim*ElemDim, Eigen::Dynamic>& dx_dxi) {
    
    uint_t
    nq      = fe_basis.n_q_points(),
    n_nodes = c.n_nodes();

    dx_dxi      = Eigen::Matrix<NodalScalarType, SpatialDim*ElemDim, Eigen::Dynamic>::Zero(SpatialDim*ElemDim, nq);

    // quadrature point spatial coordinate derivatives dx/dxi
    for (uint_t i=0; i<nq; i++) {
        
        Eigen::Map<typename Eigen::Matrix<NodalScalarType, SpatialDim*ElemDim, Eigen::Dynamic>>
        dxdxi(dx_dxi.col(i).data(), SpatialDim, ElemDim);

        for (uint_t l=0; l<n_nodes; l++)
            for (uint_t j=0; j<SpatialDim; j++)
                for (uint_t k=0; k<ElemDim; k++)
                    dxdxi(j, k) += fe_basis->dphi_dxi(i, l, k) * node_coord(j, l);
    }

}


template <typename NodalScalarType,
          uint_t ElemDim,
          uint_t SpatialDim>
inline void
compute_detJ(const Eigen::Matrix<NodalScalarType, SpatialDim*ElemDim, Eigen::Dynamic>& dx_dxi,
             Eigen::Matrix<NodalScalarType, Eigen::Dynamic, 1>& detJ) {
    
    uint_t
    nq      = dx_dxi.cols();

    detJ        = Eigen::Matrix<NodalScalarType, Eigen::Dynamic, 1>::Zero(nq);
    
    for (uint_t i=0; i<nq; i++) {
        
        Eigen::Map<const typename Eigen::Matrix<NodalScalarType, SpatialDim*ElemDim, Eigen::Dynamic>>
        dxdxi(dx_dxi.col(i).data(), SpatialDim, ElemDim);

        // determinant of dx_dxi
        detJ(i) = dxdxi.determinant();
    }
}



template <typename NodalScalarType,
          uint_t ElemDim,
          uint_t SpatialDim,
          typename ContextType>
inline void
compute_detJ_side(const ContextType& c,
                  const uint_t s,
                  const Eigen::Matrix<NodalScalarType, SpatialDim*ElemDim, Eigen::Dynamic>& dx_dxi,
                  Eigen::Matrix<NodalScalarType, Eigen::Dynamic, 1>& detJ);
    



template <typename NodalScalarType,
          uint_t ElemDim,
          uint_t SpatialDim,
          typename ContextType>
inline
typename std::enable_if<ElemDim == SpatialDim && ElemDim == 1, void>::type
compute_detJ_side
(const ContextType& c,
 const uint_t s,
 const Eigen::Matrix<NodalScalarType, ElemDim*SpatialDim, Eigen::Dynamic>& dx_dxi,
 Eigen::Matrix<NodalScalarType, Eigen::Dynamic, 1>& detJ) {
    
    Assert1(dx_dxi.cols() == 1, dx_dxi.cols(), "Only one quadrature point on side of 1D element");
    detJ        = Eigen::Matrix<NodalScalarType, Eigen::Dynamic, 1>::Ones(1);
}



inline uint_t quad_side_Jac_row(uint_t s) {
    
    // identify row of the Jacobian matrix that will be used to compute
    // the size
    switch (s) {
        case 0:
        case 2:
            return 0; // (dx/dxi, dy/dxi)
            break;
            
        case 1:
        case 3:
            return 1; // (dx/deta, dy/deta)
            break;
            
        default:
            Error(false, "Invalid side number for quad");
    }
}



template <typename NodalScalarType,
          uint_t ElemDim,
          uint_t SpatialDim,
          typename ContextType>
inline
typename std::enable_if<ElemDim == SpatialDim && ElemDim == 2, void>::type
compute_detJ_side_quad
(const ContextType& c,
 const uint_t s,
 const Eigen::Matrix<NodalScalarType, ElemDim*SpatialDim, Eigen::Dynamic>& dx_dxi,
 Eigen::Matrix<NodalScalarType, Eigen::Dynamic, 1>& detJ) {
    
    Assert0(c.elem_is_quad(), "Element must be a quadrilateral");
    
    uint_t
    nq      = dx_dxi.cols(),
    row     = quad_side_Jac_row(s);
    
    detJ        = Eigen::Matrix<NodalScalarType, Eigen::Dynamic, 1>::Zero(nq);

    for (uint_t i=0; i<nq; i++) {
        
        Eigen::Map<const typename Eigen::Matrix<NodalScalarType, 2, 2>>
        dxdxi(dx_dxi.row(i).data(), SpatialDim, ElemDim);
        
        // determinant of dx_dxi
        detJ(i) = dxdxi.row(row).norm();
    }
}



template <typename NodalScalarType,
          uint_t ElemDim,
          uint_t SpatialDim,
          typename ContextType>
inline
typename std::enable_if<ElemDim == SpatialDim && ElemDim == 2, void>::type
compute_detJ_side
 (const ContextType& c,
  const uint_t s,
  const Eigen::Matrix<NodalScalarType, ElemDim*SpatialDim, Eigen::Dynamic>& dx_dxi,
  Eigen::Matrix<NodalScalarType, Eigen::Dynamic, 1>& detJ) {
     
     if (c.elem_is_quad())
         compute_detJ_side_quad<NodalScalarType, ElemDim, SpatialDim, ContextType>(c, s);
     else
         Error(false, "Not implemented for element type.");
 }



template <typename NodalScalarType,
          uint_t ElemDim,
          uint_t SpatialDim,
          typename ContextType>
inline void
compute_side_tangent_and_normal
(const ContextType& c,
 const uint_t s,
 const Eigen::Matrix<NodalScalarType, SpatialDim*ElemDim, Eigen::Dynamic>& dx_dxi,
 Eigen::Matrix<NodalScalarType, SpatialDim, Eigen::Dynamic>&               tangent,
 Eigen::Matrix<NodalScalarType, SpatialDim, Eigen::Dynamic>&               normal);



template <typename NodalScalarType,
          uint_t ElemDim,
          uint_t SpatialDim,
          typename ContextType>
inline
typename std::enable_if<ElemDim == SpatialDim && ElemDim == 1, void>::type
compute_side_tangent_and_normal
 (const ContextType& c,
  const uint_t s,
  const Eigen::Matrix<NodalScalarType, SpatialDim*ElemDim, Eigen::Dynamic>& dx_dxi,
  Eigen::Matrix<NodalScalarType, SpatialDim, Eigen::Dynamic>&         tangent,
  Eigen::Matrix<NodalScalarType, SpatialDim, Eigen::Dynamic>&         normal) {
    
     // side of 1D is a 0-d element, so shoudl have a single point
     Assert1(dx_dxi.cols() == 1, dx_dxi.cols(), "Only one quadrature point on side of 1D element");

     normal  = Eigen::Matrix<NodalScalarType, 1, Eigen::Dynamic>::Zero(1, 1);
     tangent = Eigen::Matrix<NodalScalarType, 1, Eigen::Dynamic>::Zero(1, 1);
     
     // left side normal is -1 and right side normal is +1
     normal(0, 0) = s==0?-1.:1.;
}



template <typename NodalScalarType,
          uint_t ElemDim,
          uint_t SpatialDim,
          typename ContextType>
inline
typename std::enable_if<ElemDim == SpatialDim && ElemDim == 2, void>::type
compute_quad_side_tangent_and_normal
(const ContextType& c,
 const uint_t s,
 const Eigen::Matrix<NodalScalarType, ElemDim*SpatialDim, Eigen::Dynamic>& dx_dxi,
 Eigen::Matrix<NodalScalarType, SpatialDim, Eigen::Dynamic>&               tangent,
 Eigen::Matrix<NodalScalarType, SpatialDim, Eigen::Dynamic>&               normal) {
    
    Assert0(c.elem_is_quad(), "Element must be a quadrilateral");

    uint_t
    nq      = dx_dxi.cols(),
    row     = quad_side_Jac_row(s);
    
    tangent = Eigen::Matrix<NodalScalarType, SpatialDim, Eigen::Dynamic>::Zero(SpatialDim, nq);
    normal  = Eigen::Matrix<NodalScalarType, SpatialDim, Eigen::Dynamic>::Zero(SpatialDim, nq);

    // for bottom and right edges, the tangent is d{x, y}/dxi and d{x, y}/deta.
    // for top and left edges, the tangent is -d{x, y}/dxi and -d{x, y}/deta.
    real_t
    v       = s>1?-1.:1;
    
    Eigen::Matrix<NodalScalarType, 2, 1>
    dx = Eigen::Matrix<NodalScalarType, 2, 1>::Zero(2);
    
    for (uint_t i=0; i<nq; i++) {
        
        Eigen::Map<typename Eigen::Matrix<NodalScalarType, 2, 2>::type>
        dxdxi(dx_dxi.row(i).data(), 2, 2);
        
        // tangent
        dx(0)     = v * dxdxi(row, 0);
        dx(1)     = v * dxdxi(row, 1);
        dx       /= dx.norm();
        tangent(i, 0) =  dx(0);
        tangent(i, 1) = -dx(1);
        
        // normal is tangent cross k_hat
        // dn = |  i   j   k   | = i ty - j tx
        //      | tx  ty   0   |
        //      | 0    0   1   |
        normal(i, 0) =  dx(1);
        normal(i, 1) = -dx(0);
    }
}



template <typename NodalScalarType,
          uint_t ElemDim,
          uint_t SpatialDim,
          typename ContextType>
inline
typename std::enable_if<ElemDim == SpatialDim && ElemDim == 2, void>::type
compute_side_tangent_and_normal
 (const ContextType& c,
  const uint_t s,
  const Eigen::Matrix<NodalScalarType, SpatialDim*ElemDim, Eigen::Dynamic>& dx_dxi,
  Eigen::Matrix<NodalScalarType, SpatialDim, Eigen::Dynamic>&               tangent,
  Eigen::Matrix<NodalScalarType, SpatialDim, Eigen::Dynamic>&               normal) {
    
     if (c.elem_is_quad())
         compute_quad_side_tangent_and_normal(c, s, tangent, normal);
     else
         Error(false, "Not implemented for element type.");
}


template <typename NodalScalarType,
          uint_t ElemDim,
          uint_t SpatialDim,
          typename FEBasisType,
          typename ContextType>
inline void
compute_detJxW(const FEBasisType& fe_basis,
               const Eigen::Matrix<NodalScalarType, 1, Eigen::Dynamic>& detJ,
               Eigen::Matrix<NodalScalarType, 1, Eigen::Dynamic>&       detJxW) {
    
    Assert2(fe_basis.n_q_points() == detJ.cols(),
            fe_basis.n_q_points(), detJ.cols(),
            "Incompatible number of quadrature points of detJ and FEBasis.");
    
    uint_t
    nq      = detJ.cols();

    detJxW      = Eigen::Matrix<NodalScalarType, Eigen::Dynamic, 1>::type::Zero(nq);
    
    for (uint_t i=0; i<nq; i++) {

        // determinant times weight
        detJxW(i) = detJ(i) * fe_basis.qp_weight(i);
    }
}



template <typename NodalScalarType,
          uint_t ElemDim,
          uint_t SpatialDim>
inline void
compute_Jac_inv
 (const Eigen::Matrix<NodalScalarType, ElemDim*SpatialDim, Eigen::Dynamic>& dx_dxi,
  Eigen::Matrix<NodalScalarType, ElemDim*SpatialDim, Eigen::Dynamic>& dxi_dx);




template <typename NodalScalarType,
          uint_t ElemDim,
          uint_t SpatialDim>
inline
typename std::enable_if<ElemDim == SpatialDim, void>::type
compute_Jac_inv
 (const Eigen::Matrix<NodalScalarType, ElemDim*ElemDim, Eigen::Dynamic>& dx_dxi,
  Eigen::Matrix<NodalScalarType, ElemDim*ElemDim, Eigen::Dynamic>& dxi_dx) {

    uint_t
    nq      = dx_dxi.cols();

    dxi_dx  = Eigen::Matrix<NodalScalarType, ElemDim*ElemDim, Eigen::Dynamic>::Zero(ElemDim*ElemDim, nq);

    for (uint_t i=0; i<nq; i++) {

        // quadrature point spatial coordinate derivatives dx/dxi
        Eigen::Map<const typename Eigen::Matrix<NodalScalarType, ElemDim, ElemDim>>
        dxdxi(dx_dxi.row(i).data(), ElemDim, ElemDim);
        Eigen::Map<const typename Eigen::Matrix<NodalScalarType, ElemDim, ElemDim>>
        dxidx(dxi_dx.row(i).data(), ElemDim, ElemDim);
        
        // compute dx/dxi
        dxidx = dxdxi.inverse();
    }
}




template <typename NodalScalarType,
          uint_t ElemDim,
          uint_t SpatialDim,
          typename FEBasisType>
inline void
compute_dphi_dx
(const FEBasisType& fe_basis,
 const Eigen::Matrix<NodalScalarType, ElemDim*SpatialDim, Eigen::Dynamic>& dxi_dx,
 Eigen::Matrix<NodalScalarType, Eigen::Dynamic, Eigen::Dynamic>& dphi_dx) {
    
    uint_t
    nq      = fe_basis.n_q_points(),
    n_basis = fe_basis.n_basis();
    
    Assert2(dxi_dx.cols() == nq,
            dxi_dx.cols(), nq,
            "Incompatible quadrature points in FEBasis and dxi_dx.");
    Assert2(dxi_dx.rows() == n_basis*SpatialDim,
            dxi_dx.rows(), n_basis*SpatialDim,
            "Incompatible rows in dxi_dx.");

    dphi_dx     = Eigen::Matrix<NodalScalarType, Eigen::Dynamic, Eigen::Dynamic>::Zero(SpatialDim*n_basis, nq);
    
    for (uint_t i=0; i<nq; i++) {
        
        // quadrature point spatial coordinate derivatives dx/dxi
        Eigen::Map<const typename Eigen::Matrix<NodalScalarType, ElemDim, SpatialDim>>
        dxidx (dxi_dx.col(i).data(), ElemDim, SpatialDim),
        dphidx(dphi_dx.col(i).data(), n_basis, SpatialDim);
        
        for (uint_t l=0; l<n_basis; l++)
            for (uint_t j=0; j<SpatialDim; j++)
                for (uint_t k=0; k<ElemDim; k++)
                    dphidx(l, j) += fe_basis->dphi_dxi(i, l, k) * dxidx(k, j);
    }
}


}  // Evaluation
}  // FEBasis
}  // MAST

#endif // __mast_fe_derivative_evaluation_h__
