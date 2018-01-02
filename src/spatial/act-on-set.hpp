//
// Copyright (c) 2015-2018 CNRS
//
// This file is part of Pinocchio
// Pinocchio is free software: you can redistribute it
// and/or modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation, either version
// 3 of the License, or (at your option) any later version.
//
// Pinocchio is distributed in the hope that it will be
// useful, but WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// General Lesser Public License for more details. You should have
// received a copy of the GNU Lesser General Public License along with
// Pinocchio If not, see
// <http://www.gnu.org/licenses/>.

#ifndef __se3_act_on_set_hpp__
#define __se3_act_on_set_hpp__

#include "pinocchio/macros.hpp"
#include "pinocchio/spatial/fwd.hpp"

namespace se3
{
  namespace forceSet
  {
    ///
    /// \brief SE3 action on a set of forces, represented by a 6xN matrix whose each
    ///        column represent a spatial force.
    ///
    template<typename Mat,typename MatRet>
    static void se3Action(const SE3 & m,
                          const Eigen::MatrixBase<Mat> & iF,
                          Eigen::MatrixBase<MatRet> const & jF);
    
    ///
    /// \brief Inverse SE3 action on a set of forces, represented by a 6xN matrix whose each
    ///        column represent a spatial force.
    ///
    template<typename Mat,typename MatRet>
    static void se3ActionInverse(const SE3 & m,
                                 const Eigen::MatrixBase<Mat> & iF,
                                 Eigen::MatrixBase<MatRet> const & jF);
    
    ///
    /// \brief Action of a motion on a set of forces, represented by a 6xN matrix whose each
    ///        column represent a spatial force.
    ///
    template<typename MotionDerived, typename Mat,typename MatRet>
    static void motionAction(const MotionDense<MotionDerived> & v,
                             const Eigen::MatrixBase<Mat> & iF,
                             Eigen::MatrixBase<MatRet> const & jF);
  }  // namespace forceSet

  namespace motionSet
  {
    ///
    /// \brief SE3 action on a set of motions, represented by a 6xN matrix whose each
    ///        column represent a spatial motion.
    ///
    template<typename Mat,typename MatRet>
    static void se3Action(const SE3 & m,
                          const Eigen::MatrixBase<Mat> & iV,
                          Eigen::MatrixBase<MatRet> const & jV);
    
    ///
    /// \brief Inverse SE3 action on a set of motions, represented by a 6xN matrix whose each
    ///        column represent a spatial motion.
    ///
    template<typename Mat,typename MatRet>
    static void se3ActionInverse(const SE3 & m,
                                 const Eigen::MatrixBase<Mat> & iV,
                                 Eigen::MatrixBase<MatRet> const & jV);
    
    ///
    /// \brief Action of a motion on a set of motions, represented by a 6xN matrix whose each
    ///        column represent a spatial motion.
    ///
    template<typename MotionDerived, typename Mat,typename MatRet>
    static void motionAction(const MotionDense<MotionDerived> & v,
                             const Eigen::MatrixBase<Mat> & iF,
                             Eigen::MatrixBase<MatRet> const & jF);
  }  // namespace MotionSet

  /* --- DETAILS --------------------------------------------------------- */

  namespace internal 
  {

    template<typename Mat,typename MatRet, int NCOLS>
    struct ForceSetSe3Action
    {
      /* Compute jF = jXi * iF, where jXi is the dual action matrix associated
       * with m, and iF, jF are matrices whose columns are forces. The resolution
       * is done by block operation. It is less efficient than the colwise
       * operation and should not be used. */ 
      static void run(const SE3 & m,
                      const Eigen::MatrixBase<Mat> & iF,
                      Eigen::MatrixBase<MatRet> const & jF);
      
    };
    
    template<typename MotionDerived, typename Mat,typename MatRet, int NCOLS>
    struct ForceSetMotionAction
    {
      /* Compute dF = v ^ F, where  is the dual action operation associated
       * with v, and F, dF are matrices whose columns are forces. */
      static void run(const MotionDense<MotionDerived> & v,
                      const Eigen::MatrixBase<Mat> & iF,
                      Eigen::MatrixBase<MatRet> const & jF);
      
    };

    template<typename Mat,typename MatRet>
    struct ForceSetSe3Action<Mat,MatRet,1>
    {
      /* Compute jF = jXi * iF, where jXi is the dual action matrix associated with m,
       * and iF, jF are vectors. */
      static void run(const SE3 & m,
                      const Eigen::MatrixBase<Mat> & iF,
                      Eigen::MatrixBase<MatRet> const & jF)
      { 
        EIGEN_STATIC_ASSERT_VECTOR_ONLY(Mat);
        EIGEN_STATIC_ASSERT_VECTOR_ONLY(MatRet);
        Eigen::VectorBlock<const Mat,3> linear = iF.template head<3>();
        Eigen::VectorBlock<const Mat,3> angular = iF.template tail<3>();
        
        const_cast<Eigen::MatrixBase<MatRet> &>(jF).template head <3>() = m.rotation()*linear;
        const_cast<Eigen::MatrixBase<MatRet> &>(jF).template tail <3>() = (m.translation().cross(jF.template head<3>())
                                  + m.rotation()*angular);
      }
    };
    
    template<typename MotionDerived, typename Mat, typename MatRet>
    struct ForceSetMotionAction<MotionDerived,Mat,MatRet,1>
    {
      static void run(const MotionDense<MotionDerived> & v,
                      const Eigen::MatrixBase<Mat> & iF,
                      Eigen::MatrixBase<MatRet> const & jF)
      {
        EIGEN_STATIC_ASSERT_VECTOR_ONLY(Mat);
        EIGEN_STATIC_ASSERT_VECTOR_ONLY(MatRet);
        
        typedef ForceRef<Mat> ForceRefOnMat;
        typedef ForceRef<MatRet> ForceRefOnMatRet;
        ForceRefOnMatRet fout(const_cast<Eigen::MatrixBase<MatRet> &>(jF).derived());
        ForceRefOnMat(iF.derived()).motionAction(v,fout);
      }
    };

    /* Specialized implementation of block action, using colwise operation.  It
     * is empirically much faster than the true block operation, although I do
     * not understand why. */
    template<typename Mat,typename MatRet,int NCOLS>
    void ForceSetSe3Action<Mat,MatRet,NCOLS>::
    run(const SE3 & m,
        const Eigen::MatrixBase<Mat> & iF,
        Eigen::MatrixBase<MatRet> const & jF)
    {
      for(int col=0;col<jF.cols();++col) 
      {
        typename MatRet::ColXpr jFc
        = const_cast<Eigen::MatrixBase<MatRet> &>(jF).col(col);
        forceSet::se3Action(m,iF.col(col),jFc);
      }
    }
    
    template<typename MotionDerived, typename Mat,typename MatRet,int NCOLS>
    void ForceSetMotionAction<MotionDerived,Mat,MatRet,NCOLS>::
    run(const MotionDense<MotionDerived> & v,
        const Eigen::MatrixBase<Mat> & iF,
        Eigen::MatrixBase<MatRet> const & jF)
    {
      for(int col=0;col<jF.cols();++col)
      {
//        typename Mat::ColXpr iFc = const_cast<Eigen::MatrixBase<Mat> &>(iF).col(col);
        typename MatRet::ColXpr jFc
        = const_cast<Eigen::MatrixBase<MatRet> &>(jF).col(col);
        forceSet::motionAction(v,iF.col(col),jFc);
//        v.motionAction(MotionRefOnMat(iF.col(col)),MotionRefOnMatRet(jF.col(col)));
//        v.motionAction(MotionRefOnMat(iFc),MotionRefOnMatRet(jF.col(col)));
      }
    }
    
    template<typename Mat,typename MatRet, int NCOLS>
    struct ForceSetSe3ActionInverse
    {
      /* Compute jF = jXi * iF, where jXi is the dual action matrix associated
       * with m, and iF, jF are matrices whose columns are forces. The resolution
       * is done by block operation. It is less efficient than the colwise
       * operation and should not be used. */
      static void run(const SE3 & m,
                      const Eigen::MatrixBase<Mat> & iF,
                      Eigen::MatrixBase<MatRet> const & jF);
      
    };
    
    template<typename Mat,typename MatRet>
    struct ForceSetSe3ActionInverse<Mat,MatRet,1>
    {
      /* Compute jF = jXi * iF, where jXi is the dual action matrix associated with m,
       * and iF, jF are vectors. */
      static void run(const SE3 & m,
                      const Eigen::MatrixBase<Mat> & iF,
                      Eigen::MatrixBase<MatRet> const & jF)
      {
        EIGEN_STATIC_ASSERT_VECTOR_ONLY(Mat);
        EIGEN_STATIC_ASSERT_VECTOR_ONLY(MatRet);
        Eigen::VectorBlock<const Mat,3> linear = iF.template head<3>();
        Eigen::VectorBlock<const Mat,3> angular = iF.template tail<3>();
        
        const_cast<Eigen::MatrixBase<MatRet> &>(jF).template head <3>()
        = angular - m.translation().cross(linear); // use as temporary variable (TODO: check performance gains)
        const_cast<Eigen::MatrixBase<MatRet> &>(jF).template tail <3>()
        = m.rotation().transpose()* jF.template head <3>();
        const_cast<Eigen::MatrixBase<MatRet> &>(jF).template head <3>()
        = m.rotation().transpose()*linear;
      }
      
    };
    
    /* Specialized implementation of block action, using colwise operation.  It
     * is empirically much faster than the true block operation, although I do
     * not understand why. */
    template<typename Mat,typename MatRet,int NCOLS>
    void ForceSetSe3ActionInverse<Mat,MatRet,NCOLS>::
    run(const SE3 & m,
        const Eigen::MatrixBase<Mat> & iF,
        Eigen::MatrixBase<MatRet> const & jF)
    {
      for(int col=0;col<jF.cols();++col)
      {
        typename MatRet::ColXpr jFc
        = const_cast<Eigen::MatrixBase<MatRet> &>(jF).col(col);
        forceSet::se3ActionInverse(m,iF.col(col),jFc);
      }
    }

  } // namespace internal

  namespace forceSet
  {
    template<typename Mat,typename MatRet>
    static void se3Action(const SE3 & m,
                          const Eigen::MatrixBase<Mat> & iF,
                          Eigen::MatrixBase<MatRet> const & jF)
    {
      internal::ForceSetSe3Action<Mat,MatRet,Mat::ColsAtCompileTime>::run(m,iF,jF);
    }
    
    template<typename Mat,typename MatRet>
    static void se3ActionInverse(const SE3 & m,
                                 const Eigen::MatrixBase<Mat> & iF,
                                 Eigen::MatrixBase<MatRet> const & jF)
    {
      internal::ForceSetSe3ActionInverse<Mat,MatRet,Mat::ColsAtCompileTime>::run(m,iF,jF);
    }
    
    template<typename MotionDerived, typename Mat,typename MatRet>
    static void motionAction(const MotionDense<MotionDerived> & v,
                             const Eigen::MatrixBase<Mat> & iF,
                             Eigen::MatrixBase<MatRet> const & jF)
    {
      internal::ForceSetMotionAction<MotionDerived,Mat,MatRet,Mat::ColsAtCompileTime>::run(v,iF,jF);
    }

  }  // namespace forceSet


  namespace internal 
  {

    template<typename Mat,typename MatRet, int NCOLS>
    struct MotionSetSe3Action
    {
      /* Compute jF = jXi * iF, where jXi is the action matrix associated
       * with m, and iF, jF are matrices whose columns are motions. */
      static void run(const SE3 & m,
                      const Eigen::MatrixBase<Mat> & iF,
                      Eigen::MatrixBase<MatRet> const & jF);
      
    };
    
    template<typename MotionDerived, typename Mat,typename MatRet, int NCOLS>
    struct MotionSetMotionAction
    {
      /* Compute dV = v ^ V, where  is the action operation associated
       * with v, and V, dV are matrices whose columns are motions. */
      static void run(const MotionDense<MotionDerived> & v,
                      const Eigen::MatrixBase<Mat> & iV,
                      Eigen::MatrixBase<MatRet> const & jV);
      
    };

    template<typename Mat,typename MatRet>
    struct MotionSetSe3Action<Mat,MatRet,1>
    {
      /* Compute jV = jXi * iV, where jXi is the action matrix associated with m,
       * and iV, jV are 6D vectors representing spatial velocities. */
      static void run(const SE3 & m,
                      const Eigen::MatrixBase<Mat> & iV,
                      Eigen::MatrixBase<MatRet> const & jV)
      { 
        EIGEN_STATIC_ASSERT_VECTOR_ONLY(Mat);
        EIGEN_STATIC_ASSERT_VECTOR_ONLY(MatRet);
        Eigen::VectorBlock<const Mat,3> linear = iV.template head<3>();
        Eigen::VectorBlock<const Mat,3> angular = iV.template tail<3>();
        
        /* ( R*v + px(Rw),  Rw ) */
        const_cast<Eigen::MatrixBase<MatRet> &>(jV).template tail <3>()
        = m.rotation()*angular;
        const_cast<Eigen::MatrixBase<MatRet> &>(jV).template head <3>()
        = (m.translation().cross(jV.template tail<3>()) + m.rotation()*linear);
      }
    };
    
    template<typename MotionDerived, typename Mat,typename MatRet>
    struct MotionSetMotionAction<MotionDerived,Mat,MatRet,1>
    {
      static void run(const MotionDense<MotionDerived> & v,
                      const Eigen::MatrixBase<Mat> & iV,
                      Eigen::MatrixBase<MatRet> const & jV)
      {
        EIGEN_STATIC_ASSERT_VECTOR_ONLY(Mat);
        EIGEN_STATIC_ASSERT_VECTOR_ONLY(MatRet);
        
        typedef MotionRef<Mat> MotionRefOnMat;
        typedef MotionRef<MatRet> MotionRefOnMatRet;
        MotionRefOnMatRet mout(const_cast<Eigen::MatrixBase<MatRet> &>(jV).derived());
        MotionRefOnMat(iV.derived()).motionAction(v,mout);
      }
    };

    /* Specialized implementation of block action, using colwise operation.  It
     * is empirically much faster than the true block operation, although I do
     * not understand why. */
    template<typename Mat,typename MatRet,int NCOLS>
    void MotionSetSe3Action<Mat,MatRet,NCOLS>::
    run(const SE3 & m,
        const Eigen::MatrixBase<Mat> & iV,
        Eigen::MatrixBase<MatRet> const & jV)
    {
      for(int col=0;col<jV.cols();++col)
      {
        typename MatRet::ColXpr jVc
        = const_cast<Eigen::MatrixBase<MatRet> &>(jV).col(col);
        motionSet::se3Action(m,iV.col(col),jVc);
      }
    }
    
    template<typename MotionDerived, typename Mat,typename MatRet,int NCOLS>
    void MotionSetMotionAction<MotionDerived,Mat,MatRet,NCOLS>::
    run(const MotionDense<MotionDerived> & v,
        const Eigen::MatrixBase<Mat> & iV,
        Eigen::MatrixBase<MatRet> const & jV)
    {
      for(int col=0;col<jV.cols();++col)
      {
        typename MatRet::ColXpr jVc
        = const_cast<Eigen::MatrixBase<MatRet> &>(jV).col(col);
        motionSet::motionAction(v,iV.col(col),jVc);
      }
    }
    
    template<typename Mat,typename MatRet, int NCOLS>
    struct MotionSetSe3ActionInverse
    {
      /* Compute jF = jXi * iF, where jXi is the action matrix associated
       * with m, and iF, jF are matrices whose columns are motions. The resolution
       * is done by block operation. It is less efficient than the colwise
       * operation and should not be used. */
      static void run(const SE3 & m,
                      const Eigen::MatrixBase<Mat> & iF,
                      Eigen::MatrixBase<MatRet> const & jF);
      
    };
    
    template<typename Mat,typename MatRet>
    struct MotionSetSe3ActionInverse<Mat,MatRet,1>
    {
      /* Compute jV = jXi * iV, where jXi is the action matrix associated with m,
       * and iV, jV are 6D vectors representing spatial velocities. */
      static void run(const SE3 & m,
                      const Eigen::MatrixBase<Mat> & iV,
                      Eigen::MatrixBase<MatRet> const & jV)
      {
        EIGEN_STATIC_ASSERT_VECTOR_ONLY(Mat);
        EIGEN_STATIC_ASSERT_VECTOR_ONLY(MatRet);
        Eigen::VectorBlock<const Mat,3> linear = iV.template head<3>();
        Eigen::VectorBlock<const Mat,3> angular = iV.template tail<3>();
        
        const_cast<Eigen::MatrixBase<MatRet> &>(jV).template tail <3>()
        = linear - m.translation().cross(angular); // use as temporary variable
        const_cast<Eigen::MatrixBase<MatRet> &>(jV).template head <3>()
        = m.rotation().transpose()*jV.template tail<3>();
        const_cast<Eigen::MatrixBase<MatRet> &>(jV).template tail <3>()
        = m.rotation().transpose()*angular;
      }
    };
    
    /* Specialized implementation of block action, using colwise operation.  It
     * is empirically much faster than the true block operation, although I do
     * not understand why. */
    template<typename Mat,typename MatRet,int NCOLS>
    void MotionSetSe3ActionInverse<Mat,MatRet,NCOLS>::
    run(const SE3 & m,
        const Eigen::MatrixBase<Mat> & iV,
        Eigen::MatrixBase<MatRet> const & jV)
    {
      for(int col=0;col<jV.cols();++col)
      {
        typename MatRet::ColXpr jVc
        = const_cast<Eigen::MatrixBase<MatRet> &>(jV).col(col);
        motionSet::se3ActionInverse(m,iV.col(col),jVc);
      }
    }
    
    

  } // namespace internal

  namespace motionSet
  {
    template<typename Mat,typename MatRet>
    static void se3Action(const SE3 & m,
                          const Eigen::MatrixBase<Mat> & iV,
                          Eigen::MatrixBase<MatRet> const & jV)
    {
      internal::MotionSetSe3Action<Mat,MatRet,Mat::ColsAtCompileTime>::run(m,iV,jV);
    }
    
    template<typename Mat,typename MatRet>
    static void se3ActionInverse(const SE3 & m,
                                 const Eigen::MatrixBase<Mat> & iV,
                                 Eigen::MatrixBase<MatRet> const & jV)
    {
      internal::MotionSetSe3ActionInverse<Mat,MatRet,Mat::ColsAtCompileTime>::run(m,iV,jV);
    }
    
    template<typename MotionDerived, typename Mat,typename MatRet>
    static void motionAction(const MotionDense<MotionDerived> & v,
                             const Eigen::MatrixBase<Mat> & iV,
                             Eigen::MatrixBase<MatRet> const & jV)
    {
      internal::MotionSetMotionAction<MotionDerived,Mat,MatRet,Mat::ColsAtCompileTime>::run(v,iV,jV);
    }

  }  // namespace motionSet


} // namespace se3

#endif // ifndef __se3_act_on_set_hpp__
