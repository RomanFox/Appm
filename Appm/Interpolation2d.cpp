#include "Interpolation2d.h"

Interpolation2d::Interpolation2d()
{
}



Interpolation2d::Interpolation2d(const std::string & filename, const DataTransform xTrans, const DataTransform yTrans, const DataTransform fTrans, const double yScale, const double fScale)
{
	this->xTrans = xTrans;
	this->yTrans = yTrans;
	this->fTrans = fTrans;

	// Read data from file
	readCsvFile(filename, xTrans, yTrans, fTrans, yScale, fScale);

	//writeCsvFile("temp.dat");

	assert(this->n = x.length());
	assert(this->m = y.length());
	assert(n * m == f.length());

	// Build spline
	assert(this->m >= 2);
	assert(this->n >= 2);
	assert(this->d >= 1);
	alglib::spline2dbuildbicubicv(x, n, y, m, f, d, s);
}

Interpolation2d::~Interpolation2d()
{
}

/**
* Interpolate data using bicubic splines z = S(x,y).
*
* @param xSites vector of x(i)
* @param ySites vector of y(i)
* @return vector z(i) = S(x(i), y(i))
*/
const Eigen::VectorXd Interpolation2d::bicubicInterp(const Eigen::VectorXd & xSites, const Eigen::VectorXd & ySites) const
{
	assert(this->m > 0 && this->n > 0);

	const int n = xSites.size();
	assert(n == ySites.size());
	Eigen::VectorXd result(n);

	// data transformation to x is not implmented
	assert(xTrans == DataTransform::NONE);

	// Apply transformation on y-axis
	Eigen::VectorXd ySitesTrans = ySites;
	switch (yTrans) {
	case DataTransform::NONE:
		// do nothing
		break;

	case DataTransform::INVERSE:
		ySitesTrans = ySites.array().inverse();
		break;

	case DataTransform::LOG:
		ySitesTrans = ySites.array().exp();
		break;

	default:
		assert(false);
	}

	// Interpolate data for each element in list supplied
	for (int i = 0; i < n; i++) {
		result(i) = alglib::spline2dcalc(s, xSites(i), ySitesTrans(i));
	}

	// Apply transformation on function data
	Eigen::VectorXd resultTrans = result;
	switch (fTrans) {
	case DataTransform::NONE:
		// do noting
		break;

	case DataTransform::LOG:
		resultTrans = result.array().exp();
		break;
		
	case DataTransform::INVERSE:
		resultTrans = result.array().inverse();
		break;
		
	default:
		assert(false);
	}
	return resultTrans;
}

/**
* Write data to file.
* @param filename    filename of output file
*/
void Interpolation2d::writeData(const std::string & filename, const double yScale, const double fScale) const
{
	std::cout << "Write data to file: " << filename << std::endl;
	int n;

	n = x.length();
	Eigen::VectorXd tempX(n);
	tempX.setZero();
	for (int i = 0; i < n; i++) {
		tempX(i) = *(x.getcontent() + i);
	}

	int m;
	m = y.length();
	Eigen::VectorXd tempY(m);
	tempY.setZero();
	for (int i = 0; i < m; i++) {
		tempY(i) = *(y.getcontent() + i);
	}

	int k;
	k = f.length();
	Eigen::VectorXd tempF(k);
	tempF.setZero();
	for (int i = 0; i < k; i++) {
		tempF(i) = *(f.getcontent() + i);
	}

	assert(n*m == k);

	// back-transform data
	assert(fTrans == DataTransform::LOG);
	Eigen::VectorXd fResult(tempF.size());
	fResult = tempF;
	if (fTrans == DataTransform::LOG) {
		fResult = tempF.array().exp() / fScale;
	}
	typedef Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> MatrixXdRowMajor;
	Eigen::Map<MatrixXdRowMajor> fMap(fResult.data(), m, n);

	assert(yTrans == DataTransform::INVERSE);
	Eigen::VectorXd yResult(tempY.size());
	yResult = tempY;
	if (yTrans == DataTransform::INVERSE) {
		yResult = tempY.array().inverse() / yScale;
	}

	assert(xTrans == DataTransform::NONE);
	Eigen::VectorXd xResult(tempX.size());
	xResult = tempX;

	// Write data to output matrix
	Eigen::MatrixXd outData = Eigen::MatrixXd::Zero(m + 1, n + 1);
	outData.bottomRightCorner(m, n) = fMap;
	outData.bottomLeftCorner(m, 1) = yResult;
	for (int i = 0; i < n; i++) {
		outData(0, i + 1) = xResult(i);
	}

	std::ofstream file(filename);
	file << outData << std::endl;
	std::cout << "DONE" << std::endl;

}



/**
* Read 2d data file with format given as follows:
* M = [a x; y z];
* where a is a scalar (dummy value), x is a row vector, 
* y is a column vector, and z is a matrix, 
* such that f(x(j), y(i)) = z(i,j). That is, z(i,j) are the grid values
* to be interpolated.
*/
void Interpolation2d::readCsvFile(const std::string & filename, const DataTransform & xTrans, const DataTransform & yTrans, const DataTransform & fTrans, const double yScale, const double fScale)
{
	assert(xTrans == DataTransform::NONE);

	std::ifstream file(filename);
	assert(file.is_open());

	// Read each line of data file
	std::string line;
	int rows = 0;
	int cols = 0;
	std::vector<double> data;

	while(std::getline(file, line)) 
	{
		if (line.empty() || line.front() == '#') {
			continue; // skip empty lines and comment lines
		}

		// Read a single line 
		std::string chunk;
		std::stringstream ss(line);
		while (std::getline(ss, chunk, ',')) {
			double value = std::stod(chunk);
			data.push_back(value);
		}

		// get number of columns
		if (rows == 0) {
			cols = data.size();
		}
		rows++;
	}
	assert(data.size() == (rows * cols));

	// Note that we read data in row-major format. 
	typedef Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> MatrixXdRowMajor;
	Eigen::Map<MatrixXdRowMajor> map(data.data(), rows, cols);
	
	Eigen::VectorXd firstRow = map.row(0);
	Eigen::VectorXd firstCol = map.col(0);

	Eigen::VectorXd lambdaVec = firstRow.segment(1, firstRow.size() - 1);
	Eigen::VectorXd TeVec = firstCol.segment(1, firstCol.size() - 1);

	// assert that values in lambdaVec and TeVec are montonic increasing
	assert(isSorted(lambdaVec));
	assert(isSorted(TeVec));

	TeVec *= yScale;

	MatrixXdRowMajor dataMatrix = map.bottomRightCorner(rows - 1, cols - 1);
	assert(dataMatrix.allFinite());
	dataMatrix *= fScale;
	assert(dataMatrix.allFinite());
	if (fTrans != DataTransform::NONE) {
		dataMatrix = applyTransform(dataMatrix, fTrans);
	}
	if (yTrans != DataTransform::NONE) {
		TeVec = applyTransform(TeVec, yTrans);
	}
	assert(dataMatrix.allFinite());
	assert(TeVec.allFinite());

	this->f.setcontent(dataMatrix.size(), dataMatrix.data());
	this->y.setcontent(TeVec.size(), TeVec.data());

	//Eigen::VectorXd TeVec_inv = TeVec.array().inverse();
	//MatrixXdRowMajor dataMatrix_log = dataMatrix.array().log();
	//this->f.setcontent(dataMatrix_log.size(), dataMatrix_log.data());
	//this->y.setcontent(TeVec_inv.size(), TeVec_inv.data());


	// Set variables for spline data
	assert(lambdaVec.allFinite());
	this->x.setcontent(lambdaVec.size(), lambdaVec.data());

	this->n = this->x.length();
	this->m = this->y.length();
	assert(this->n > 0);
	assert(this->m > 0);
}

void Interpolation2d::writeCsvFile(const std::string & filename)
{
	std::ofstream file(filename);
	for (int j = 0; j < x.length(); j++) {
		file << x(j) << ",";
	}
	file << std::endl;

	for (int i = 0; i < 1; i++) {
		for (int j = 0; j < x.length(); j++) {
			file << f(i*x.length() + j) << ",";
		}
		file << std::endl;
	}

}

/**
* If data vector is sorted in ascending order.
* @param data   data vector with at least 2 elements
* @return if data vector is sorted
*/
bool Interpolation2d::isSorted(const Eigen::VectorXd & data)
{
	assert(data.size() > 1);
	const int n = data.size();
	const Eigen::VectorXd temp = data.bottomRows(n - 1) - data.topRows(n - 1);
	const bool isSorted = (temp.array() > 0).all();
	if (!isSorted) {
		std::cout << "Data vector is not sorted: " << std::endl;
		std::cout << data << std::endl;
	}
	return isSorted;
}

template <class T>
T Interpolation2d::applyTransform(const T array, const DataTransform & trans)
{
	T transformed = array;
	if (trans == DataTransform::INVERSE) {
		transformed = array.array().inverse();
	}
	if (trans == DataTransform::LOG) {
		transformed = array.array().log();
	}
	return transformed;
}
