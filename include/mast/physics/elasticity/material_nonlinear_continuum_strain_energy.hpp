/*
* MAST: Multidisciplinary-design Adaptation and Sensitivity Toolkit
* Copyright (C) 2013-2020  Manav Bhatia and MAST authors
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef __mast_material_nonlinear_continuum_strain_energy_h__
#define __mast_material_nonlinear_continuum_strain_energy_h__

// MAST includes
#include <mast/physics/elasticity/linear_elastic_strain_operator.hpp>

namespace MAST {
namespace Physics {
namespace Elasticity {
namespace ElastoPlasticity {


template <typename FEVarType,
          typename YieldSurfaceType,
          uint_t Dim>
class StrainEnergy {
    
public:

    using scalar_t         = typename FEVarType::scalar_t;
    using basis_scalar_t   = typename FEVarType::fe_shape_deriv_t::scalar_t;
    using vector_t         = typename Eigen::Matrix<scalar_t, Eigen::Dynamic, 1>;
    using matrix_t         = typename Eigen::Matrix<scalar_t, Eigen::Dynamic, Eigen::Dynamic>;
    using fe_shape_deriv_t = typename FEVarType::fe_shape_deriv_t;
    static const uint_t
    n_strain               = MAST::Physics::Elasticity::LinearContinuum::NStrainComponents<Dim>::value;

    StrainEnergy():
    _yield       (nullptr),
    _fe_var_data (nullptr)
    { }
    
    virtual ~StrainEnergy() { }

    inline void
    set_yield_surface(const YieldSurfaceType& ys) {
        
        Assert0(!_yield, "Yield surface already initialized.");
        
        _yield = &ys;
    }

    inline void set_fe_var_data(const FEVarType& fe_data)
    {
        Assert0(!_fe_var_data, "FE data already initialized.");
        _fe_var_data = &fe_data;
    }

    inline uint_t n_dofs() const {

        Assert0(_fe_var_data, "FE data not initialized.");

        return Dim*_fe_var_data->get_fe_shape_data().n_basis();
    }
    
    template <typename ContextType>
    inline void compute(ContextType& c,
                        vector_t& res,
                        matrix_t* jac = nullptr) const {
        
        Assert0(_fe_var_data, "FE data not initialized.");
        Assert0(_yield, "Yield surface not initialized");
        
        const typename FEVarType::fe_shape_deriv_t
        &fe = _fe_var_data->get_fe_shape_data();
        
        typename Eigen::Matrix<scalar_t, n_strain, 1>
        epsilon,
        stress;
        vector_t
        vec     = vector_t::Zero(Dim*fe.n_basis());
        
        typename YieldSurfaceType::stiff_t
        mat;
        
        matrix_t
        mat1 = matrix_t::Zero(n_strain, Dim*fe.n_basis()),
        mat2 = matrix_t::Zero(Dim*fe.n_basis(), Dim*fe.n_basis());

        MAST::Numerics::FEMOperatorMatrix<scalar_t>
        Bxmat;
        Bxmat.reinit(n_strain, Dim, fe.n_basis());

        
        for (uint_t i=0; i<fe.n_q_points(); i++) {
            
            c.init_for_qp(i);
            
            MAST::Physics::Elasticity::LinearContinuum::strain
            <scalar_t, scalar_t, FEVarType, Dim>(*_fe_var_data, i, epsilon, Bxmat);
            
            // evaluate the stress and tangent stiffness matrix
            _yield->compute(c, epsilon, *c.mp_accessor, &mat);

            Bxmat.vector_mult_transpose(vec, stress);
            res += fe.detJxW(i) * vec;
            
            if (jac) {
                
                Bxmat.left_multiply(mat1, mat);
                Bxmat.right_multiply_transpose(mat2, mat1);
                (*jac) += fe.detJxW(i) * mat2;
            }
        }
    }

    template <typename ContextType, typename ScalarFieldType>
    inline void derivative(ContextType& c,
                           const ScalarFieldType& f,
                           vector_t& res,
                           matrix_t* jac = nullptr) const {
        
        Assert0(_fe_var_data, "FE data not initialized.");
        Assert0(_yield, "Yield surface not initialized");
        
        const typename FEVarType::fe_shape_deriv_t
        &fe = _fe_var_data->get_fe_shape_data();

        typename Eigen::Matrix<scalar_t, n_strain, 1>
        epsilon,
        stress;
        vector_t
        vec     = vector_t::Zero(Dim*fe.n_basis());

        typename YieldSurfaceType::stiff_t
        mat;
        matrix_t
        mat1 = matrix_t::Zero(n_strain, Dim*fe.n_basis()),
        mat2 = matrix_t::Zero(Dim*fe.n_basis(), Dim*fe.n_basis());

        MAST::Numerics::FEMOperatorMatrix<scalar_t>
        Bxmat;
        Bxmat.reinit(n_strain, Dim, fe.n_basis());

        
        for (uint_t i=0; i<fe.n_q_points(); i++) {
            
            c.init_for_qp(i);

            MAST::Physics::Elasticity::LinearContinuum::strain
            <scalar_t, scalar_t, FEVarType, Dim>(*_fe_var_data, i, epsilon, Bxmat);
            
            //_yield->derivative(c, f, stress, &mat);

            Bxmat.vector_mult_transpose(vec, stress);
            res += fe.detJxW(i) * vec;
            
            if (jac) {
                
                Bxmat.left_multiply(mat1, mat);
                Bxmat.right_multiply_transpose(mat2, mat1);
                (*jac) += fe.detJxW(i) * mat2;
            }
        }
    }

    
private:
    
    
    const YieldSurfaceType          *_yield;
    const FEVarType                 *_fe_var_data;
};

}  // namespace MaterialNonlinear
}  // namespace Elasticity
}  // namespace Physics
}  // namespace MAST

#endif // __mast_material_nonlinear_continuum_strain_energy_h__
