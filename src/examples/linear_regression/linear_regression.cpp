#define EIGEN_USE_MKL_ALL
//#define MLT_VERBOSE_TRAINING

#include <iostream>
#include <iomanip>

#include <Eigen/Eigen>

#include "../../examples/misc/misc.hpp"
#include "../../mlt/models/regressors/least_squares_linear_regressor.hpp"
#include "../../mlt/trainers/gradient_based/gradient_descent.hpp"

using namespace std;
using namespace Eigen;

using namespace mlt::models::regressors;
using namespace mlt::trainers::gradient_based;

struct Params {
	struct GradientDescent {
		static constexpr int epochs = 400;
		static constexpr int batch_size = 0;
		static constexpr double learning_rate = 0.01;
		static constexpr double learning_rate_decay = 1;
		static constexpr gradient_descent_update_t update_method = gradient_descent_update_t::gradient_descent;
		static constexpr double update_param = 0;
	};
};

tuple<MatrixXd, MatrixXd> house_value_dataset() {
	auto input_v = parse_csv<double>("house_data.csv", ',');
	MatrixXd input(input_v.size(), 3);
	VectorXd target(input_v.size(), 1);

	auto i = 0;
	for (const vector<double>& row : input_v) {
		input(i, 0) = 1;
		for (auto j = 0; j < 2; j++) {
			input(i, j + 1) = row[j];
		}
		target(i) = row[2];
		i++;
	}
	
	return make_tuple(input, target);
}

inline VectorXd correlatedData(double x) {
	return (VectorXd(4) << 1, x, 2 * x, 0.5 * x*x).finished();
}

inline VectorXd correlatedTarget(double x) {
	return (VectorXd(2) << 5 * x + 3, x).finished();
}

tuple<MatrixXd, MatrixXd> correlated_data_dataset(int n) {	
	VectorXd points = VectorXd::Random(n, 1) * 100;
	MatrixXd input(points.rows(), correlatedData(1).rows());
	MatrixXd target(points.rows(), correlatedTarget(1).rows());

	for (auto i = 0; i < points.rows(); i++) {
		input.row(i) = correlatedData(points(i)).topRows(input.cols());
		target.row(i) = correlatedTarget(points(i));
	}

	return make_tuple(input, target);
}

void example(tuple<MatrixXd, MatrixXd> data, VectorXd test) {
	MatrixXd input, target;

	std::tie(input, target) = data;

	cout << "First 10 examples from the dataset: " << endl;

	for (auto i = 0; i < 10; i++) {
		cout << " x = [";

		for (auto j = 0; j < input.cols() - 1; j++) {
			cout << input(i, j) << " ";
		}

		cout << input(i, input.cols() - 1) << "], y = [";

		for (auto j = 0; j < target.cols() - 1; j++) {
			cout << target(i, j) << " ";
		}

		cout << target(i, target.cols() - 1) << "]" << endl;
	}

	// Normalize input features
	RowVectorXd mean = input.rightCols(input.cols() - 1).colwise().mean();
	input.rightCols(input.cols() - 1).rowwise() -= mean;
	MatrixXd cov = (input.rightCols(input.cols() - 1).adjoint() * input.rightCols(input.cols() - 1)) / double(input.rows() - 1);
	RowVectorXd sigma = cov.diagonal().transpose().cwiseSqrt();
	input.array().rightCols(input.cols() - 1).rowwise() /= sigma.array();
		
	LeastSquaresLinearRegressor lr1(input.cols() - 1, target.cols());
	LeastSquaresLinearRegressor lr2;

	GradientDescentTrainer<Params, LeastSquaresLinearRegressor> gdt(lr1);

	cout << "Training with Gradient Descent..." << endl;
	auto time1 = benchmark([&]() { gdt.train(input, target); }).count();
	cout << "Training with Normal Equations.." << endl;
	auto time2 = benchmark([&]() { lr2.self_train(input, target); }).count();

	cout << endl;
	cout << "Train Time: \t" << time1 << "ms\t" << time2 << "ms" << endl << endl;
	
	MatrixXd params(lr1.params_size(), 2);
	params.col(0) = lr1.params();
	params.col(1) = lr2	.params();		
	
	cout << "Params Found: " << endl << params << endl << endl;
	cout << "Train Cost: \t" << lr1.cost(input, target) << "\t" << lr2.cost(input, target) << endl << endl;

	MatrixXd gradients(lr1.params_size(), 2);
	gradients.col(0) = lr1.cost_gradient(input, target);
	gradients.col(1) = lr2.cost_gradient(input, target);

	cout << "Cost Gradient: " << endl << gradients << endl << endl;

	MatrixXd predictions(target.cols(), 2);
	VectorXd test_norm = test;
	test_norm.bottomRows(input.cols() - 1) -= mean.transpose();
	test_norm.bottomRows(input.cols() - 1) = test_norm.bottomRows(input.cols() - 1).cwiseQuotient(sigma.transpose());

	predictions.col(0) = lr1.regress_single(test_norm);
	predictions.col(1) = lr2.regress_single(test_norm);
	cout << "Prediction for test: " << endl << predictions << endl << endl;
	cin.get();
}

int main() {
	print_info();
	example(house_value_dataset(), Vector3d(1, 1650, 3));
	example(correlated_data_dataset(1000000), correlatedData(0));

	return 0;
}