#pragma once

namespace views {
namespace {

class GraphTime
{
public:
	static const double day;
	static const double hour;
	static const double min;
	static const double sec;
	static const double msec;

private:
  GraphTime();
};

const double GraphTime::sec	= 1.0;
const double GraphTime::min	= 60 * GraphTime::sec;
const double GraphTime::hour	= 60 * GraphTime::min;
const double GraphTime::day	= 24 * GraphTime::hour;
const double GraphTime::msec	= sec / 1000;

} // namespace
} // namespace views