/**
 * DTT - Data Transfer Tools for C++ Linear Algebra Libraries.
 * It supports data transfer between the following libraries:
 * Eigen, Armadillo, OpenCV, ArrayFire, LibTorch
 */
#pragma once

#include <iostream>
#include <cstdio>
#include <cstdlib>

#include <Eigen/Dense>
#include <armadillo>
#include <arrayfire.h>
#include <torch/torch.h>
#include <opencv2/opencv.hpp>
#include <opencv2/core/eigen.hpp>

#if CV_MAJOR_VERSION >= 4
#define CV_BGR2RGB cv::COLOR_BGR2RGB
#endif

namespace dtt {
//---------------------------------------------------------------------------
// OpenCV to Eigen, Armadillo, LibTorch, File
//---------------------------------------------------------------------------

//void cv2eigen(const Mat& src, Eigen::Matrix<_Tp, _rows, _cols, _options, _maxRows, _maxCols>& dst)

template<typename T>
Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> cv2eigen(cv::Mat &C) {
  Eigen::Map<Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>> E(C.ptr<T>(), C.rows, C.cols);
  return E;
}

template<class T>
arma::Mat<T> cv2arma(cv::Mat &cvMatIn) {
  //return arma::Mat<T>(cvMatIn.data, cvMatIn.rows, cvMatIn.cols, false, false);
  return arma::Mat<T>(reinterpret_cast<T*>(cvMatIn.data),
                      static_cast<arma::uword>(cvMatIn.cols),
                      static_cast<arma::uword>(cvMatIn.rows),
                      true,
                      true);
}

torch::Tensor cv2libtorch(cv::Mat &C, bool copydata=true, bool is_cv_image=false) {
  int kCHANNELS = C.channels();
  if (is_cv_image) {
    if (kCHANNELS == 3) {
      cv::cvtColor(C, C, CV_BGR2RGB);
      C.convertTo(C, CV_32FC3, 1.0f / 255.0f);
    } else // considering channels = 1
      C.convertTo(C, CV_32FC1, 1.0f / 255.0f);
    auto T = torch::from_blob(C.data, {1, C.rows, C.cols, kCHANNELS});
    T = T.permute({0, 3, 1, 2});
    return T;
  } else {
    std::vector<int64_t> dims;
    at::TensorOptions options(at::kFloat);
    if (kCHANNELS == 1) {
      C.convertTo(C, CV_32FC1);
      dims = {C.rows, C.cols};
    }
    else { // considering channels = 3
      C.convertTo(C, CV_32FC3);
      dims = {C.rows, C.cols, kCHANNELS};
    }
    //auto T = torch::from_blob(C.ptr<float>(), dims, options).clone();
    //auto T = torch::from_blob(C.data, at::IntList(dims), options);
    auto T = torch::from_blob(C.data, dims, options);
    if (copydata)
      return T.clone();
    else
      return T;
  }
}

void cv2file(cv::Mat M, std::string filename) {
  std::ofstream file(filename);
  if (file.is_open())
    file << M << '\n';
  file.close();
}

//---------------------------------------------------------------------------
// Eigen to Armadillo, OpenCV, File
//---------------------------------------------------------------------------

arma::mat eigen2arma(Eigen::MatrixXd& E, bool copy=true) {
  return arma::mat(E.data(), E.rows(), E.cols(), /*copy_aux_mem*/copy, /*strict*/false);
}

af::array eigen2af(Eigen::MatrixXf& E){
  af::array A(E.rows(), E.cols(), E.data());
  return A;
}

//void eigen2cv(const Eigen::Matrix<_Tp, _rows, _cols, _options, _maxRows, _maxCols>& src, cv::Mat& dst)

void eigen2file(Eigen::MatrixXf E, std::string filename) {
  std::ofstream file(filename);
  if (file.is_open())
    file << E << '\n';
  file.close();
}

//---------------------------------------------------------------------------
// ArrayFire to Eigen
//---------------------------------------------------------------------------
Eigen::MatrixXf af2eigen(af::array &A) {
  float* data = A.host<float>();
  Eigen::Map<Eigen::MatrixXf> E(data, A.dims(0), A.dims(1));
  return E;
}

//---------------------------------------------------------------------------
// Armadillo to Eigen, OpenCV, File
//---------------------------------------------------------------------------

Eigen::MatrixXd arma2eigen(arma::mat& M) {
  return Eigen::Map<Eigen::MatrixXd>(M.memptr(), M.n_rows, M.n_cols);
}

template<class T>
void arma2cv(const arma::Mat<T>& arma_mat_in, cv::Mat_<T>& cv_mat_out) {
  cv::transpose(cv::Mat_<T>(static_cast<int>(arma_mat_in.n_cols),
                            static_cast<int>(arma_mat_in.n_rows),
                            const_cast<T*>(arma_mat_in.memptr())),
                cv_mat_out);
};

void arma2file(arma::mat& A, std::string filename) {
  A.save(filename);
}

//---------------------------------------------------------------------------
// LibTorch to OpenCV
//---------------------------------------------------------------------------

// Consider torch::Tensor as a float matrix
cv::Mat libtorch2cv(torch::Tensor &Tin, bool copy=true) {
  auto T = Tin.to(torch::kCPU);
  cv::Mat C;
  if (copy) {
    cv::Mat M(T.size(0), T.size(1), CV_32FC1, T.data<float>());
    M.copyTo(C);
  } else
    C = cv::Mat(T.size(0), T.size(1), CV_32FC1, T.data<float>());
  return C;
}

}
