#include <math.h>
#include <stdarg.h>
#include <algorithm>
#include <iostream>
#include <vector>

const double eps = 10e-5;
const double degree_to_rad = M_PI / 180;

bool isMatch(double x, double y) { return fabs(x - y) < eps; }

class Line;

struct Point {
  double x = 0.0;
  double y = 0.0;
  Point(double x, double y) : x(x), y(y) {}
  Point() = default;
  Point(const Point& other) = default;
  Point& operator=(const Point& other) = default;
  Point& operator+=(const Point& another);
  void rotate(const Point& center, double angle);
  void rotate(const Point& center, double sin, double cos);
  void reflect(const Point& center);
  void reflect(const Line& axis);
  void scale(const Point& center, double coefficient);
  double norm() const;
  Point& operator*=(double coeff);
  Point& operator/=(double coeff);
  double distance(const Point& other) const;
  Point center(const Point& other) const;
};

class Line {
  double a_;
  double b_;
  double c_;

 public:
  Line() = default;
  Line(double a, double b, double c);
  Line(const Point& first, const Point& second);
  Line(double coeff, double shift);
  Line(const Point& point, double coeff);
  double getPointValue(const Point& point) const;
  bool isPointLie(const Point& point) const;
  double getA() const;
  double getB() const;
  double getC() const;
  Point getNormal() const;
  double distance(const Point& point) const;
  static Point intersection(const Line& first, const Line& second);
  static bool isCollinear(const Line& first, const Line& second);
};

std::ostream& operator<<(std::ostream& out, const Point& point) {
  out << "(" << point.x << ", " << point.y << ")";
  return out;
}
bool operator==(const Point& first, const Point& second) {
  return isMatch(first.x, second.x) && isMatch(first.y, second.y);
}
bool operator!=(const Point& first, const Point& second) {
  return !(first == second);
}

double Point::norm() const { return sqrt(x * x + y * y); }
Point& Point::operator+=(const Point& another) {
  x += another.x;
  y += another.y;
  return *this;
}
Point& Point::operator*=(double coeff) {
  if (!isMatch(x, 0.0)) {
    x *= coeff;
  }
  if (!isMatch(y, 0.0)) {
    y *= coeff;
  }
  return *this;
}
Point& Point::operator/=(double coeff) {
  *this *= (1 / coeff);
  return *this;
}
Point operator*(const Point& point, double coeff) {
  Point res(point);
  res *= coeff;
  return res;
}
Point operator-(const Point& first, const Point& second) {
  return Point(first.x - second.x, first.y - second.y);
}
Point operator+(const Point& first, const Point& second) {
  Point ans(first);
  ans += second;
  return ans;
}
void Point::rotate(const Point& center, double angle) {
  angle *= degree_to_rad;
  double new_x =
      (x - center.x) * cos(angle) - (y - center.y) * sin(angle) + center.x;
  double new_y =
      (x - center.x) * sin(angle) + (y - center.y) * cos(angle) + center.y;
  x = new_x;
  y = new_y;
}
void Point::rotate(const Point& center, double sin, double cos) {
  double new_x = (x - center.x) * cos - (y - center.y) * sin + center.x;
  double new_y = (x - center.x) * sin + (y - center.y) * cos + center.y;
  x = new_x;
  y = new_y;
}
void Point::reflect(const Point& center) {
  if (*this == center) {
    return;
  }
  *this = center + (center - *this);
}
void Point::reflect(const Line& axis) {
  if (axis.isPointLie(*this)) {
    return;
  }
  Point normal = axis.getNormal();
  Point shift = normal *= (2.0 * axis.distance(*this) / normal.norm());
  if (axis.getPointValue(*this) > 0) {
    shift *= -1;
  }
  *this += shift;
}
void Point::scale(const Point& center, double coefficient) {
  if (*this == center) {
    return;
  }
  *this = center + ((*this - center) *= coefficient);
}
double Point::distance(const Point& other) const {
  return (*this - other).norm();
}
Point Point::center(const Point& other) const {
  Point new_point = *this;
  new_point += other;
  new_point /= 2;
  return new_point;
}

Line::Line(double a, double b, double c) : a_(a), b_(b), c_(c) {}
Line::Line(const Point& first, const Point& second) {
  if (isMatch(first.x, second.x)) {
    a_ = 1.0;
    b_ = 0.0;
    c_ = -first.x;
  } else if (isMatch(first.y, second.y)) {
    a_ = 0.0;
    b_ = 1.0;
    c_ = -first.y;
  } else {
    a_ = 1.0;
    b_ = (second.x - first.x) / (first.y - second.y);
    c_ = -(first.x + b_ * first.y);
  }
}
Line::Line(double coeff, double shift) : a_(coeff), b_(-1.0), c_(shift) {}
Line::Line(const Point& point, double coeff)
    : a_(coeff), b_(-1.0), c_(point.y - point.x * coeff) {}
double Line::getPointValue(const Point& point) const {
  return a_ * point.x + b_ * point.y + c_;
}
bool Line::isPointLie(const Point& point) const {
  return isMatch(getPointValue(point), 0.0);
}
double Line::getA() const { return a_; }
double Line::getB() const { return b_; }
double Line::getC() const { return c_; }
Point Line::getNormal() const {
  Point res(a_, b_);
  return res;
}
double Line::distance(const Point& point) const {
  return (fabs(a_ * point.x + b_ * point.y + c_) / this->getNormal().norm());
}
Point Line::intersection(const Line& first, const Line& second) {
  double big_delta = first.a_ * second.b_ - first.b_ * second.a_;
  double x_delta = -(first.c_ * second.b_ - first.b_ * second.c_);
  double y_delta = -(first.a_ * second.c_ - first.c_ * second.a_);
  Point ans(x_delta / big_delta, y_delta / big_delta);
  return ans;
}
bool Line::isCollinear(const Line& first, const Line& second) {
  return isMatch(first.a_ * second.b_ - first.b_ * second.a_, 0.0);
}

bool operator==(const Line& first, const Line& second) {
  bool a_zero_first = isMatch(first.getA(), 0.0);
  bool a_zero_second = isMatch(second.getA(), 0.0);
  if (a_zero_first ^ a_zero_second) {
    return false;
  }
  if (a_zero_first) {
    return isMatch(first.getC() * second.getB() / first.getB(), second.getC());
  }
  return isMatch(first.getB() * second.getA() / first.getA(), second.getB()) &&
         isMatch(first.getC() * second.getA() / first.getA(), second.getC());
}
bool operator!=(const Line& first, const Line& second) {
  return !(first == second);
}

class Shape {
 public:
  virtual double area() const = 0;
  virtual double perimeter() const = 0;
  virtual bool containsPoint(const Point& point) const = 0;
  virtual void rotate(const Point& center, double angle) = 0;
  virtual void reflect(const Point& center) = 0;
  virtual void reflect(const Line& axis) = 0;
  virtual void scale(const Point& center, double coefficient) = 0;
  virtual ~Shape() = default;
  virtual bool operator==(const Shape& another) const;
  virtual bool isCongruentTo(const Shape& another) const;
  virtual bool isSimilarTo(const Shape& another) const;
};

class Polygon : public Shape {
 protected:
  std::vector<Point> vertices_;

 public:
  Polygon() = default;
  template <typename... T>
  explicit Polygon(T&&... vertices) : vertices_{std::forward<T>(vertices)...} {}
  ~Polygon() = default;
  const std::vector<Point>& getVertices() const { return vertices_; }
  std::vector<Point> getVerticesCopy() const {
    std::vector<Point> res(vertices_);
    return res;
  }
  std::vector<Point> getSides() const {
    std::vector<Point> sides;
    for (size_t i = 0; i < vertices_.size() - 1; ++i) {
      sides.push_back(vertices_[i + 1] - vertices_[i]);
    }
    sides.push_back(vertices_[0] - vertices_[vertices_.size() - 1]);
    return sides;
  }
  std::vector<double> getSidesSquare() const {
    std::vector<Point> sides(getSides());
    std::vector<double> result;
    result.push_back(sides[0].x * sides[sides.size() - 1].y -
                     sides[0].y * sides[sides.size() - 1].x);
    for (size_t i = 0; i < sides.size() - 1; ++i) {
      result.push_back(sides[i + 1].x * sides[i].y -
                       sides[i + 1].y * sides[i].x);
    }
    return result;
  }
  std::vector<double> getSidesSinuses() const {
    std::vector<Point> sides(getSides());
    std::vector<double> squares(getSidesSquare());
    squares[0] /= sides[0].norm();
    squares[0] /= sides[sides.size() - 1].norm();
    for (size_t i = 1; i < squares.size(); ++i) {
      squares[i] /= sides[i - 1].norm();
      squares[i] /= sides[i].norm();
    }
    return squares;
  }
  size_t verticesCount() const { return vertices_.size(); }
  bool isConvex() const {
    bool signPositive;
    for (size_t i = 1; i < vertices_.size(); ++i) {
      Line line_temp(vertices_[i - 1], vertices_[i]);
      signPositive = line_temp.getPointValue(vertices_[0]) > 0;
      if (i == 1) {
        signPositive = line_temp.getPointValue(vertices_[2]) > 0;
      }
      for (size_t j = 1; j < i - 1; ++j) {
        if (line_temp.getPointValue(vertices_[j]) > 0 != signPositive) {
          return false;
        }
      }
      for (size_t j = (i != 1 ? i + 1 : 3); j < vertices_.size(); ++j) {
        if (line_temp.getPointValue(vertices_[j]) > 0 != signPositive) {
          return false;
        }
      }
    }
    Line line_temp(vertices_[vertices_.size() - 1], vertices_[0]);
    signPositive = line_temp.getPointValue(vertices_[1]) > 0;
    for (size_t j = 2; j < vertices_.size() - 1; ++j) {
      if (line_temp.getPointValue(vertices_[j]) > 0 != signPositive) {
        return false;
      }
    }
    return true;
  }
  double perimeter() const override {
    double ans = 0.0;
    for (size_t i = 0; i < vertices_.size() - 1; ++i) {
      ans += vertices_[i].distance(vertices_[i + 1]);
    }
    ans += vertices_[vertices_.size() - 1].distance(vertices_[0]);
    return ans;
  }
  double area() const override {
    double ans = 0.0;
    size_t size = vertices_.size();
    ans += vertices_[size - 1].x * vertices_[0].y;
    ans -= vertices_[size - 1].y * vertices_[0].x;
    for (size_t i = 0; i < size - 1; ++i) {
      ans += vertices_[i].x * vertices_[i + 1].y;
      ans -= vertices_[i].y * vertices_[i + 1].x;
    }
    ans /= 2;
    ans = fabs(ans);
    return ans;
  }
  bool containsPoint(const Point& point) const override {
    Line ray(point, point + Point(1, 1));
    int intersection_cnt = 0;
    size_t size = vertices_.size();
    for (size_t i = 0; i < size; ++i) {
      if (vertices_[i] == point) {
        return true;
      }
      Line check_line = i != size - 1 ? Line(vertices_[i], vertices_[i + 1])
                                      : Line(vertices_[size - 1], vertices_[0]);
      if (!Line::isCollinear(check_line, ray)) {
        if (!ray.isPointLie(vertices_[i])) {
          Point intersect_point = Line::intersection(check_line, ray);
          if ((intersect_point.x >= point.x) &&
              (vertices_[i + 1].y - intersect_point.y) *
                      (intersect_point.y - vertices_[i].y) >=
                  0 &&
              (vertices_[i + 1].x - intersect_point.x) *
                      (intersect_point.x - vertices_[i].x) >=
                  0) {
            ++intersection_cnt;
          }
        }
      }
    }
    return intersection_cnt % 2 == 1;
  }
  void rotate(const Point& center, double angle) override {
    for (size_t i = 0; i < vertices_.size(); ++i) {
      vertices_[i].rotate(center, angle);
    }
  }
  void reflect(const Point& center) override {
    for (size_t i = 0; i < vertices_.size(); ++i) {
      vertices_[i].reflect(center);
    }
  }
  void reflect(const Line& axis) override {
    for (size_t i = 0; i < vertices_.size(); ++i) {
      vertices_[i].reflect(axis);
    }
  }
  void scale(const Point& center, double coefficient) override {
    for (size_t i = 0; i < vertices_.size(); ++i) {
      vertices_[i].scale(center, coefficient);
    }
  }
  bool operator==(const Shape& other) const override;
  bool operator!=(const Shape& other) const { return !(*this == other); }
  bool isCongruentTo(const Shape& other) const override;
  bool isSimilarTo(const Shape& other) const override;
};

std::ostream& operator<<(std::ostream& out, const Polygon& poly) {
  const std::vector<Point>& vertices(poly.getVertices());
  out << "POLY:" << std::endl;
  for (size_t i = 0; i < vertices.size(); ++i) {
    out << vertices[i] << std::endl;
  }
  return out;
}

class Ellipse : public Shape {
 protected:
  std::pair<Point, Point> focuses_;
  double minor_axe_;
  double major_axe_;
  double focal_;

 public:
  Ellipse(const Point& first, const Point& second, double distance)
      : focuses_(std::pair<Point, Point>(first, second)),
        major_axe_(distance / 2) {
    focal_ = first.distance(second) / 2;
    minor_axe_ = sqrt(major_axe_ * major_axe_ - focal_ * focal_);
  }
  ~Ellipse() = default;
  std::pair<Point, Point> focuses() const { return focuses_; }
  std::pair<Line, Line> directrices() const {
    Point first_line_point = focuses_.first;
    Point second_line_point = focuses_.second;
    Point center_point = this->center();
    double coeff = major_axe_ * major_axe_ / (focal_ * focal_);
    first_line_point.scale(center_point, coeff);
    second_line_point.scale(center_point, coeff);
    Point shift = focuses_.first - center_point;
    shift.rotate(Point(0.0, 0.0), 90.0);
    std::pair<Line, Line> ans;
    ans.first = Line(first_line_point, first_line_point + shift);
    ans.second = Line(second_line_point, second_line_point + shift);
    return ans;
  }
  double eccentricity() const { return focal_ / major_axe_; }
  Point center() const {
    Point ans(focuses_.first);
    ans += focuses_.second;
    ans *= 0.5;
    return ans;
  }
  double area() const override {
    double ans = M_PI * major_axe_ * minor_axe_;
    return ans;
  }
  double perimeter() const override {
    double temp = (major_axe_ - minor_axe_) * (major_axe_ - minor_axe_);
    temp /= (major_axe_ + minor_axe_) * (major_axe_ + minor_axe_);
    double res = M_PI * (major_axe_ + minor_axe_) *
                 (1.0 + (3.0 * temp) / (10.0 + sqrt(4.0 - 3.0 * temp)));
    return res;
  }
  bool containsPoint(const Point& point) const override {
    double dist_sum =
        point.distance(focuses_.first) + point.distance(focuses_.second);
    if (dist_sum - 2 * major_axe_ < eps) {
      return true;
    }
    return false;
  }
  void rotate(const Point& center, double angle) override {
    focuses_.first.rotate(center, angle);
    focuses_.second.rotate(center, angle);
  }
  void reflect(const Point& center) override {
    focuses_.first.reflect(center);
    focuses_.second.reflect(center);
  }
  void reflect(const Line& axis) override {
    focuses_.first.reflect(axis);
    focuses_.second.reflect(axis);
  }
  void scale(const Point& center, double coefficient) override {
    focal_ *= coefficient;
    major_axe_ *= coefficient;
    minor_axe_ *= coefficient;
    focuses_.first.scale(center, coefficient);
    focuses_.second.scale(center, coefficient);
  }
  double getMajorAxe() const { return major_axe_; }
  double getMinorAxe() const { return minor_axe_; }
  double getFocal() const { return focal_; }
  bool operator==(const Shape& other) const override;
  bool operator!=(const Shape& other) const { return !(*this == other); }
  bool isCongruentTo(const Shape& other) const override;
  bool isSimilarTo(const Shape& other) const override;
};

class Circle : public Ellipse {
 public:
  Circle(const Point& center, double radius)
      : Ellipse(center, center, radius * 2) {}
  Circle() : Ellipse(Point(0, 0), Point(0, 0), 0) {}
  ~Circle() = default;
  Point center() const { return Ellipse::focuses_.first; }
  double radius() const { return major_axe_; }
  double area() const override {
    double ans = M_PI * major_axe_ * major_axe_;
    return ans;
  }
  double perimeter() const override {
    double result = 2 * M_PI * major_axe_;
    return result;
  }
  bool containsPoint(const Point& point) const override {
    double dist_sum = point.distance(focuses_.first);
    if (dist_sum - major_axe_ < eps) {
      return true;
    }
    return false;
  }
};

class Triangle : public Polygon {
  using Polygon::vertices_;

 public:
  Triangle(const Point& first, const Point& second, const Point& third)
      : Polygon(Point(first), Point(second), Point(third)) {}
  ~Triangle() = default;
  Circle circumscribedCircle() const {
    Point first_line_point = vertices_[0].center(vertices_[1]);
    Point first_line_shift = vertices_[1] - vertices_[0];
    first_line_shift.rotate(Point(0.0, 0.0), 90.0);
    Point second_line_point = vertices_[1].center(vertices_[2]);
    Point second_line_shift = vertices_[2] - vertices_[1];
    second_line_shift.rotate(Point(0.0, 0.0), 90.0);
    Line first_line(first_line_point, first_line_point + first_line_shift);
    Line second_line(second_line_point, second_line_point + second_line_shift);
    Point new_center(Line::intersection(first_line, second_line));
    double new_rad = new_center.distance(vertices_[0]);
    Circle answer(new_center, new_rad);
    return answer;
  }
  Circle inscribedCircle() const {
    double side_0 = vertices_[1].distance(vertices_[2]);
    double side_1 = vertices_[0].distance(vertices_[2]);
    double side_2 = vertices_[0].distance(vertices_[1]);
    double half_sum = side_0 + side_1 + side_2;
    Point new_center =
        vertices_[0] * side_0 + vertices_[1] * side_1 + vertices_[2] * side_2;
    new_center /= half_sum;
    half_sum /= 2;
    double new_rad = sqrt((half_sum - side_0) * (half_sum - side_1) *
                          (half_sum - side_2) / half_sum);
    Circle answer(new_center, new_rad);
    return answer;
  }
  Point centroid() const {
    Point answer = vertices_[0] + vertices_[1] + vertices_[2];
    answer /= 3;
    return answer;
  }
  Point orthocenter() const {
    Point first_line_shift = vertices_[2] - vertices_[1];
    first_line_shift.rotate(Point(0.0, 0.0), 90.0);
    Point second_line_shift = vertices_[0] - vertices_[2];
    second_line_shift.rotate(Point(0.0, 0.0), 90.0);
    Line first_line(vertices_[0], vertices_[0] + first_line_shift);
    Line second_line(vertices_[1], vertices_[1] + second_line_shift);
    Point answer(Line::intersection(first_line, second_line));
    return answer;
  }
  Line EulerLine() const { return Line(centroid(), orthocenter()); }
  Circle ninePointsCircle() const {
    Triangle new_trig(vertices_[0].center(vertices_[1]),
                      vertices_[1].center(vertices_[2]),
                      vertices_[2].center(vertices_[0]));
    return new_trig.circumscribedCircle();
  }
};

std::ostream& operator<<(std::ostream& out, const Ellipse& ellipse) {
  out << "ELLIPSE:" << std::endl;
  out << "FOCUSES:" << std::endl;
  out << ellipse.focuses().first << std::endl
      << ellipse.focuses().second << std::endl;
  out << "BIG: " << ellipse.getMajorAxe() << " SMALL: " << ellipse.getMinorAxe()
      << std::endl;
  return out;
}
std::ostream& operator<<(std::ostream& out, const Line& line) {
  out << "LINE:" << std::endl;
  out << line.getA() << "x + " << line.getB() << "y + " << line.getC()
      << std::endl;
  return out;
}

class Rectangle : public Polygon {
 protected:
  using Polygon::vertices_;

 public:
  Rectangle(const Point& first, const Point& second, double coefficient) {
    vertices_.push_back(first);
    vertices_.push_back(second);
    double cos = 1 / sqrt(1.0 + coefficient * coefficient);
    double sin = coefficient * cos;
    vertices_[1].scale(first, cos);
    vertices_[1].rotate(first, sin, cos);
    vertices_.push_back(second);
    vertices_.push_back(second);
    vertices_[3].scale(first, sin);
    vertices_[3].rotate(first, -cos, sin);
  }
  Rectangle(const Point& p_1, const Point& p_2, const Point& p_3,
            const Point& p_4)
      : Polygon(p_1, p_2, p_3, p_4) {}
  ~Rectangle() = default;
  Point center() const {
    double new_x = 0.0;
    double new_y = 0.0;
    for (size_t i = 0; i < 4; ++i) {
      new_x += vertices_[i].x;
      new_y += vertices_[i].y;
    }
    return Point(new_x / 4, new_y / 4);
  }
  std::pair<Line, Line> diagonals() const {
    return std::pair<Line, Line>(Line(vertices_[0], vertices_[2]),
                                 Line(vertices_[1], vertices_[3]));
  }
  double perimeter() const override {
    return 2 * (vertices_[1].distance(vertices_[0]) +
                vertices_[2].distance(vertices_[1]));
  }
  double area() const override {
    return vertices_[1].distance(vertices_[0]) *
           vertices_[2].distance(vertices_[1]);
  }
};

class Square : public Rectangle {
 public:
  Square(const Point& first, const Point& second)
      : Rectangle(first, second, 1.0) {}
  ~Square() = default;
  Point center() const {
    Point res = vertices_[0] + vertices_[2];
    res *= 0.5;
    return res;
  }
  Circle circumscribedCircle() const {
    return Circle(center(), center().distance(vertices_[0]));
  }
  Circle inscribedCircle() const {
    return Circle(center(), 0.5 * vertices_[0].distance(vertices_[1]));
  }
  double perimeter() const override {
    return 4 * vertices_[1].distance(vertices_[0]);
  }
  double area() const override {
    double a = vertices_[1].distance(vertices_[0]);
    a *= a;
    return a;
  }
};

template <typename T>
bool checkVectorEqualOriented(const std::vector<T>& first,
                              const std::vector<T>& second) {
  if (first.size() != second.size()) {
    return false;
  }
  for (size_t i = 0; i < first.size(); ++i) {
    bool flag = true;
    for (size_t j = 0; j < second.size() - i; ++j) {
      if (second[j] != first[i + j]) {
        flag = false;
        break;
      }
    }
    for (size_t j = second.size() - i; j < second.size(); ++j) {
      if (second[j] != first[i + j - second.size()]) {
        flag = false;
        break;
      }
    }
    if (flag) {
      return true;
    }
  }
  return false;
}
template <>
bool checkVectorEqualOriented(const std::vector<double>& first,
                              const std::vector<double>& second) {
  if (first.size() != second.size()) {
    return false;
  }
  for (size_t i = 0; i < first.size(); ++i) {
    bool flag = true;
    for (size_t j = 0; j < second.size() - i; ++j) {
      if (!isMatch(second[j], first[i + j])) {
        flag = false;
        break;
      }
    }
    for (size_t j = second.size() - i; j < second.size(); ++j) {
      if (!isMatch(second[j], first[i + j - second.size()])) {
        flag = false;
        break;
      }
    }
    if (flag) {
      return true;
    }
  }
  return false;
}

template <typename T>
bool checkVectorEqual(std::vector<T> first, std::vector<T> second) {
  if (first.size() != second.size()) {
    return false;
  }
  if (checkVectorEqualOriented(first, second)) {
    return true;
  }
  std::reverse(first.begin(), first.end());
  return checkVectorEqualOriented(first, second);
}

template <>
bool checkVectorEqual(std::vector<double> first, std::vector<double> second) {
  if (first.size() != second.size()) {
    return false;
  }
  if (checkVectorEqualOriented(first, second)) {
    return true;
  }
  for (size_t i = 0; i < first.size(); ++i) {
    second[i] *= -1;
  }
  if (checkVectorEqualOriented(first, second)) {
    return true;
  }
  std::reverse(second.begin(), second.end());
  if (checkVectorEqualOriented(first, second)) {
    return true;
  }
  for (size_t i = 0; i < first.size(); ++i) {
    second[i] *= -1;
  }
  return checkVectorEqualOriented(first, second);
}

bool Polygon::operator==(const Shape& other) const {
  const Shape* other_ptr = &other;
  const Polygon* other_poly = dynamic_cast<const Polygon*>(other_ptr);
  if (other_poly == nullptr) {
    return false;
  }
  return checkVectorEqual(getVerticesCopy(), other_poly->getVerticesCopy());
}
bool Ellipse::operator==(const Shape& other) const {
  const Shape* other_ptr = &other;
  const Ellipse* other_ellipse = dynamic_cast<const Ellipse*>(other_ptr);
  if (other_ellipse == nullptr) {
    return false;
  }
  return focuses_.first == other_ellipse->focuses_.first &&
         focuses_.second == other_ellipse->focuses_.second &&
         isMatch(getMajorAxe(), other_ellipse->getMajorAxe());
}
bool Shape::operator==(const Shape& another) const {
  const Polygon* this_poly = dynamic_cast<const Polygon*>(this);
  if (this_poly == nullptr) {
    const Ellipse* this_ellipse = dynamic_cast<const Ellipse*>(this);
    return *this_ellipse == another;
  }
  return *this_poly == another;
}
bool Ellipse::isCongruentTo(const Shape& other) const {
  const Shape* other_ptr = &other;
  const Ellipse* other_ellipse = dynamic_cast<const Ellipse*>(other_ptr);
  if (other_ellipse == nullptr) {
    return false;
  }
  return isMatch(getMinorAxe(), other_ellipse->getMinorAxe()) &&
         isMatch(getMajorAxe(), other_ellipse->getMajorAxe());
}
bool Polygon::isCongruentTo(const Shape& other) const {
  const Shape* other_ptr = &other;
  const Polygon* other_poly = dynamic_cast<const Polygon*>(other_ptr);
  if (other_poly == nullptr) {
    return false;
  }
  std::vector<double> this_vect(getSidesSquare());
  std::vector<double> other_vect(other_poly->getSidesSquare());
  return checkVectorEqual(this_vect, other_vect);
}
bool Shape::isCongruentTo(const Shape& another) const {
  const Polygon* this_poly = dynamic_cast<const Polygon*>(this);
  if (this_poly == nullptr) {
    const Ellipse* this_ellipse = dynamic_cast<const Ellipse*>(this);
    return *this_ellipse == another;
  }
  return *this_poly == another;
}
bool Ellipse::isSimilarTo(const Shape& other) const {
  const Shape* other_ptr = &other;
  const Ellipse* other_ellipse = dynamic_cast<const Ellipse*>(other_ptr);
  if (other_ellipse == nullptr) {
    return false;
  }
  return isMatch(getMajorAxe() / getMinorAxe(),
                 other_ellipse->getMajorAxe() / other_ellipse->getMinorAxe());
}
bool Polygon::isSimilarTo(const Shape& other) const {
  const Shape* other_ptr = &other;
  const Polygon* other_poly = dynamic_cast<const Polygon*>(other_ptr);
  if (other_poly == nullptr) {
    return false;
  }
  std::vector<double> this_vect(getSidesSinuses());
  std::vector<double> other_vect(other_poly->getSidesSinuses());
  return checkVectorEqual(this_vect, other_vect);
}
bool Shape::isSimilarTo(const Shape& another) const {
  const Polygon* this_poly = dynamic_cast<const Polygon*>(this);
  if (this_poly == nullptr) {
    const Ellipse* this_ellipse = dynamic_cast<const Ellipse*>(this);
    return *this_ellipse == another;
  }
  return *this_poly == another;
}
