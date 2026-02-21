
#include "catch2/catch_approx.hpp"
#define CATCH_CONFIG_MAIN

#define WINDOWS_LEAN_AND_MEAN

#include <catch2/catch_test_macros.hpp>
#include <Gorgon/Geometry/Transform3D.h>
#include <Gorgon/Geometry/Point3D.h>
#include <Gorgon/Geometry/Point.h>
#include <Gorgon/Geometry/Size.h>
#include <Gorgon/Geometry/Bounds.h>
#include <Gorgon/Geometry/Rectangle.h>
#include <Gorgon/String.h>

#undef Rectangle

#pragma warning(disable:4305)
#pragma warning(disable:4244)

using namespace Gorgon::Geometry;

using Catch::Approx;

TEST_CASE( "Point constructors", "[Point]" ) {

	Point  p1(1.2, 2.2);
	Pointf p2(1.2, 2.2);
	Point  p3(p2);
	Pointf p4(p3);
	Point  p5;
	Pointf p6;
	Pointf p7;
	Point  p8=Point::FromVector(4, 0.785398f, p1);   //45 degrees
	Pointf p9=Pointf::FromVector(4.1, 0.785398f, p2); //45 degrees
	Point  p10("1, 2");
	Pointf p11("1.2, 2.2");

	p5=p2;
	p6=p2;
	p7=p1;

	REQUIRE( p1.X == 1);
	REQUIRE( p1.Y == 2);

	REQUIRE( p2.X == Approx(1.2f) );
	REQUIRE( p2.Y == Approx(2.2f) );

	REQUIRE( p3.X == 1);
	REQUIRE( p3.Y == 2);

	REQUIRE( p4.X == Approx(1) );
	REQUIRE( p4.Y == Approx(2) );

	REQUIRE( p5.X == 1);
	REQUIRE( p5.Y == 2);

	REQUIRE( p6.X == Approx(1.2f) );
	REQUIRE( p6.Y == Approx(2.2f) );

	REQUIRE( p7.X == Approx(1) );
	REQUIRE( p7.Y == Approx(2) );

	REQUIRE( p8.X == 3);
	REQUIRE( p8.Y == 4);

	REQUIRE( p9.X == Approx(4.099138f) );
	REQUIRE( p9.Y == Approx(5.099138f) );

	REQUIRE( p10.X == 1);
	REQUIRE( p10.Y == 2);

	REQUIRE( p11.X == Approx(1.2f) );
	REQUIRE( p11.Y == Approx(2.2f) );
}

TEST_CASE( "Point comparison", "[Point]" ) {
	Point  p1(1, 2);
	Pointf p2(1.2f, 2.2f);
	Point  p3(1, 2);
	Pointf p4(1.2f, 2.2f);
	Point  p5(3, 4);
	Pointf p6(3.3f, 7.1f);

	REQUIRE( p1 == p3 );
	REQUIRE( p2 == p4 );

	REQUIRE_FALSE( p1 == p5 );
	REQUIRE_FALSE( p2 == p6 );

	REQUIRE_FALSE( p1 != p3 );
	REQUIRE_FALSE( p2 != p4 );

	REQUIRE( p1 != p5 );
	REQUIRE( p2 != p6 );

	REQUIRE( p1.Compare(p1) );
	REQUIRE( p1.Compare(p3) );
	REQUIRE_FALSE( p1.Compare(p5) );
	REQUIRE( p1 == p1 );
	REQUIRE_FALSE( p1 != p1 );

	REQUIRE( p2.Compare(p2) );
	REQUIRE( p2.Compare(p4) );
	REQUIRE_FALSE( p2.Compare(p6) );
	REQUIRE( p2 == p2 );
	REQUIRE_FALSE( p2 != p2 );
}

TEST_CASE( "Point arithmetic", "[Point]" ) {

	Point  p1(1, 2);
	Pointf p2(1.2f, 2.2f);
	Point  p3(5, 6);
	Pointf p4(5.5f, 6.8f);

	Point  p5;
	Pointf p6;

	p5=p1+p3;
	REQUIRE( p5 == Point(6,8) );

	p5=p1-p3;
	REQUIRE( p5 == Point(-4,-4) );

	p5=p1*2;
	REQUIRE( p5 == Point(2,4) );

	p5=p1/2;
	REQUIRE( p5 == Point(0,1) );

	p5=p1*1.5;
	REQUIRE( p5 == Point(1,3) );

	p5=p1/0.5;
	REQUIRE( p5 == Point(2,4) );

	p6=p2+p4;
	REQUIRE( p6.X == Approx(6.7f) );
	REQUIRE( p6.Y == Approx(9.0f) );

	p6=p2-p4;
	REQUIRE( p6.X == Approx(-4.3f) );
	REQUIRE( p6.Y == Approx(-4.6f) );

	p6=p2*2;
	REQUIRE( p6.X == Approx(2.4f) );
	REQUIRE( p6.Y == Approx(4.4f) );

	p6=p2/2;
	REQUIRE( p6.X == Approx(0.6f) );
	REQUIRE( p6.Y == Approx(1.1f) );

	p6=p2*1.5;
	REQUIRE( p6.X == Approx(1.8f) );
	REQUIRE( p6.Y == Approx(3.3f) );

	p6=p2/0.5;
	REQUIRE( p6.X == Approx(2.4f) );
	REQUIRE( p6.Y == Approx(4.4f) );



	p5.Move(p1.X, p1.Y);
	REQUIRE( p5 == p1 );
	p5+=p3;
	REQUIRE( p5 == Point(6,8) );

	p5=p1;
	p5-=p3;
	REQUIRE( p5 == Point(-4,-4) );

	p5=p1;
	p5*=2;
	REQUIRE( p5 == Point(2,4) );

	p5=p1;
	p5/=2;
	REQUIRE( p5 == Point(0,1) );

	p5=p1;
	p5*=1.5;
	REQUIRE( p5 == Point(1,3) );

	p5=p1;
	p5/=0.5;
	REQUIRE( p5 == Point(2,4) );

	p6.Move(p2.X, p2.Y);
	REQUIRE( p6 == p2 );
	p6+=p4;
	REQUIRE( p6.X == Approx(6.7f) );
	REQUIRE( p6.Y == Approx(9.0f) );

	p6=p2;
	p6-=p4;
	REQUIRE( p6.X == Approx(-4.3f) );
	REQUIRE( p6.Y == Approx(-4.6f) );

	p6=p2;
	p6*=2;
	REQUIRE( p6.X == Approx(2.4f) );
	REQUIRE( p6.Y == Approx(4.4f) );

	p6=p2;
	p6/=2;
	REQUIRE( p6.X == Approx(0.6f) );
	REQUIRE( p6.Y == Approx(1.1f) );

	p6=p2;
	p6*=1.5;
	REQUIRE( p6.X == Approx(1.8f) );
	REQUIRE( p6.Y == Approx(3.3f) );

	p6=p2;
	p6/=0.5;
	REQUIRE( p6.X == Approx(2.4f) );
	REQUIRE( p6.Y == Approx(4.4f) );


	p5=-p1;
	REQUIRE( p5 == Point(-1,-2) );

	p6=-p2;
	REQUIRE( p6.X == Approx(-1.2f) );
	REQUIRE( p6.Y == Approx(-2.2f) );
}

TEST_CASE( "Point geometric info", "[Point]") {

	Point  p1(1, 2);
	Pointf p2(1.2f, 2.2f);
	Point  p3(5, 6);
	Pointf p4(5.5f, 6.8f);

	Point  p5;
	Pointf p6;

	REQUIRE( p1.Distance() == Approx(2.236068f) );
	REQUIRE( p2.Distance() == Approx(2.505992f) );

	REQUIRE( p1.Distance(p3) == Approx(5.656854f) );
	REQUIRE( p2.Distance(p4) == Approx(6.296824f) );

	REQUIRE( p1.Angle() == Approx(1.107149f) );
	REQUIRE( p2.Angle() == Approx(1.071450f) );

	REQUIRE( p1.Angle(p3) == Approx(-2.356195f) );
	REQUIRE( p2.Angle(p4) == Approx(-2.322499f) );

	REQUIRE( p1.Slope() == Approx(2.000000f) );
	REQUIRE( p2.Slope() == Approx(1.833333f) );

	REQUIRE( p1.Slope(p3) == Approx(1.000000f) );
	REQUIRE( p2.Slope(p4) == Approx(1.069767f) );
}

	/*TEST_CASE( "Point <-> string", "[Point]" ) {
	
	Point  p1("1, 2");
	Pointf p2("(1.2f, 2.2f)");
	Point  p3;
	Pointf p4;

	std::stringstream ss;
	auto makestream=[&](std::string s) -> std::stringstream & {
		p3 = {0,0};
		p4 = {0,0};
		
		ss.str(s);
		ss.clear();
		
		return ss;
	};
	
	auto resetstream=[&] {
		p3 = {0,0};
		p4 = {0,0};
		
		ss.str("");
		ss.clear();
	};
	
	
	makestream("1,2")>>p3;
	REQUIRE( p1 == p3 );
	
	makestream("1.2,2.2")>>p3;
	REQUIRE( p1 == p3 );
	
	makestream("-1.2,-2.2")>>p3;
	REQUIRE( p1 == -p3 );
	
	makestream("(1,2)")>>p3;
	REQUIRE( p1 == p3 );
	
	makestream("(1.2,2.2)")>>p3;
	REQUIRE( p1 == p3 );
	
	makestream("(-1.2,-2.2)")>>p3;
	REQUIRE( p1 == -p3 );
	
	
	makestream("1, 2")>>p3;
	REQUIRE( p1 == p3 );
	
	makestream("1.2, 2.2")>>p3;
	REQUIRE( p1 == p3 );
	
	makestream("-1.2, -2.2")>>p3;
	REQUIRE( p1 == -p3 );
	
	makestream("(1, 2)")>>p3;
	REQUIRE( p1 == p3 );
	
	makestream("(1.2, 2.2)")>>p3;
	REQUIRE( p1 == p3 );
	
	makestream("(-1.2, -2.2)")>>p3;
	REQUIRE( p1 == -p3 );
	
	
	makestream("1,2")>>p3;
	REQUIRE( p1 == p3 );
	
	makestream("1.2,2.2")>>p3;
	REQUIRE( p1 == p3 );
	
	makestream("-1.2,-2.2")>>p3;
	REQUIRE( p1 == -p3 );
	
	
	makestream("(1,2) (1.2,2.2)")>>p3>>p4;
	REQUIRE( p1 == p3 );
	REQUIRE( p2 == p4 );
	
	makestream("( 1,2 ) 1.2,2.2")>>p3>>p4;
	REQUIRE( p1 == p3 );
	REQUIRE( p2 == p4 );
	
	makestream("(1.2,2.2) (1.2,2.2)")>>p3>>p4;
	REQUIRE( p1 == p3 );
	REQUIRE( p2 == p4 );
	
	makestream("1.2,2.2 1.2,2.2")>>p3>>p4;
	REQUIRE( p1 == p3 );
	REQUIRE( p2 == p4 );
	
	makestream("(-1.2,-2.2) (-1.2,-2.2)")>>p3>>p4;
	REQUIRE( p1 == -p3 );
	REQUIRE( p2 == -p4 );

	
	REQUIRE( std::string(p1) == "(1, 2)" );
	
	//Its ok to have extra zeros at the end
	REQUIRE( Gorgon::String::Replace(std::string(p2),"0","") == "(1.2, 2.2)" );
	
	REQUIRE( std::string(-p1) == "(-1, -2)" );
	
	
	resetstream();
	ss<<p1;
	REQUIRE( Gorgon::String::Replace(ss.str(), " ","") == "(1,2)" );

	resetstream();
	ss<<p2;
	REQUIRE( Gorgon::String::Replace(ss.str(), " ","") == "(1.2,2.2)" );
	
	resetstream();
	ss<<p1;
	makestream(ss.str())>>p3;
	REQUIRE( p1 == p3 );
	
	resetstream();
	ss<<p2;
	makestream(ss.str())>>p4;
	REQUIRE( p2 == p4 );
	
	REQUIRE( Gorgon::String::To<Point>("1,2") == p1 );
	REQUIRE( Gorgon::String::To<Point>("1.2,2.2") == p1 );
	REQUIRE( Gorgon::String::To<Pointf>("1.2,2.2") == p2 );
	
	REQUIRE( Gorgon::String::To<Point>("(1,2)") == p1 );
	REQUIRE( Gorgon::String::To<Point>("(1.2,2.2)") == p1 );
	REQUIRE( Gorgon::String::To<Pointf>("(1.2,2.2)") == p2 );
	
	REQUIRE( Gorgon::String::To<Point>("(1, 2)") == p1 );
	REQUIRE( Gorgon::String::To<Point>("(1.2, 2.2)") == p1 );
	REQUIRE( Gorgon::String::To<Pointf>("(1.2, 2.2)") == p2 );
	
	REQUIRE( Point::Parse("3,5") == Point(3,5) );
	REQUIRE( Point::Parse("(3,5)") == Point(3,5) );
	REQUIRE( Point::Parse("(3, 5)") == Point(3,5) );
	REQUIRE( Point::Parse("3.2,5.8") == Point(3,5) );
	REQUIRE( Point::Parse(" 3, 5 ") == Point(3,5) );
	
	REQUIRE_THROWS( Point::Parse("") );
	REQUIRE_THROWS( Point::Parse("3,") );
	REQUIRE_THROWS( Point::Parse("3,a") );
	REQUIRE_THROWS( Point::Parse("7a,5e") );
	REQUIRE_THROWS( Point::Parse("(3,4") );
	REQUIRE_THROWS( Point::Parse("3,4)") );
}
*/
TEST_CASE( "Point geometry functions", "[Point]" ) {
	Point  p1(1, 2);
	Pointf p2(1.2f, 2.2f);
	Point  p3;
	Pointf p4;
	Point  p5{4, 2};
	Pointf p6{4.3f, 5.5f};

	p3=p1;
	Translate(p3, 1, 2);
	REQUIRE(p3 == Point(2, 4));

	Translate(p3, p5);
	REQUIRE(p3 == Point(6, 6));

	Translate(p3, -p5);
	Translate(p3, -1, -2);
	REQUIRE(p3 == p1);

	p4=p2;
	Translate(p4, 1.2f, 2.4f);
	REQUIRE(p4.X == Approx(2.4));
	REQUIRE(p4.Y == Approx(4.6));

	Translate(p4, p6);
	REQUIRE(p4.X == Approx(6.7));
	REQUIRE(p4.Y == Approx(10.1));


	Translate(p4, -p6);
	Translate(p4, -1.2, -2.4);
	REQUIRE(p4.X == Approx(p2.X));
	REQUIRE(p4.Y == Approx(p2.Y));

	
	p3=p1;
	Scale(p3, 2);
	REQUIRE(p3 == Point(2, 4));

	Scale(p3, 2, Point{1,6});
	REQUIRE(p3 == Point(3, 2));

	Scale(p3, 0.5, Point{1, 6});
	REQUIRE(p3 == Point(2, 4));

	Scale(p3, 0.5);
	REQUIRE(p3 == Point(1, 2));

	Scale(p3, 2.5);
	REQUIRE(p3 == Point(2, 5));
	
	p4=p2;
	Scale(p4, 2);
	REQUIRE(p4.X == Approx(2.4));
	REQUIRE(p4.Y == Approx(4.4));
	
	Scale(p4, 2.5, Pointf{0.4f, 1.4f});
	REQUIRE(p4.X == Approx(5.4));
	REQUIRE(p4.Y == Approx(8.9));
	
	p3=p1;
	Scale(p3, 2.0, 0.5);
	REQUIRE(p3 == Point(2, 1));
	Scale(p3, 2.0, 0.5, Point{2,3});
	REQUIRE(p3 == Point(2, 2));
	
	p4=p2;
	Scale(p4, 2.0, 0.5);
	REQUIRE(p4.X == Approx(2.4));
	REQUIRE(p4.Y == Approx(1.1));
	
	Scale(p4, 2.0, 0.5, Pointf{2.4f,3.1f});
	REQUIRE(p4.X == Approx(2.4));
	REQUIRE(p4.Y == Approx(2.1));

	p4=p1;
	Rotate(p4, 3.14159265359/2);
	REQUIRE(p4.X == Approx(-2));
	REQUIRE(p4.Y == Approx(1));
	
	p4={3, 4};
	Rotate(p4, 3.14159265359/2, Pointf{2,2});
	REQUIRE(p4.X == Approx(0));
	REQUIRE(p4.Y == Approx(3));
	
	p4=p2;
	SkewX(p4, 0.2);
	REQUIRE(p4.X == Approx(p2.X+2.2*0.2));
	REQUIRE(p4.Y == p2.Y);
	
	p4=p2;
	SkewY(p4, 0.2);
	REQUIRE(p4.X == p2.X);
	REQUIRE(p4.Y == Approx(p2.Y+1.2*0.2));
	
	p4=p2;
	SkewX(p4, 0.2, Pointf{2,2});
	REQUIRE(p4.X == Approx(p2.X+0.2*0.2));
	REQUIRE(p4.Y == p2.Y);
	
	p4=p2;
	SkewY(p4, 0.2, Pointf{2,2});
	REQUIRE(p4.X == p2.X);
	REQUIRE(p4.Y == Approx(p2.Y-0.8*0.2));

	
	p3=p1;
	ReflectY(p3);
	REQUIRE(p3 == Point(-1, 2));
	
	ReflectX(p3);
	REQUIRE(p3 == Point(-1, -2));
	
	ReflectY(p3, Point{2, 2});
	REQUIRE(p3 == Point(5, -2));
	
	ReflectX(p3, Point{2, 2});
	REQUIRE(p3 == Point(5, 6));
	
	
	p4=p2;
	ReflectY(p4);
	REQUIRE(p4.X == Approx(-1.2));
	REQUIRE(p4.Y == Approx(2.2));
	
	ReflectX(p4);
	REQUIRE(p4.X == Approx(-1.2));
	REQUIRE(p4.Y == Approx(-2.2));
	
	ReflectY(p4, Pointf{2, 2});
	REQUIRE(p4.X == Approx(5.2));
	REQUIRE(p4.Y == Approx(-2.2));
	
	ReflectX(p4, Pointf{2, 2});
	REQUIRE(p4.X == Approx(5.2));
	REQUIRE(p4.Y == Approx(6.2));

	
	p3=p1;
	VerticalMirror(p3);
	REQUIRE(p3 == Point(-1, 2));
	
	HorizontalMirror(p3);
	REQUIRE(p3 == Point(-1, -2));
	
	VerticalMirror(p3, Point{2, 2});
	REQUIRE(p3 == Point(5, -2));
	
	HorizontalMirror(p3, Point{2, 2});
	REQUIRE(p3 == Point(5, 6));
	
	
	p4=p2;
	VerticalMirror(p4);
	REQUIRE(p4.X == Approx(-1.2));
	REQUIRE(p4.Y == Approx(2.2));
	
	HorizontalMirror(p4);
	REQUIRE(p4.X == Approx(-1.2));
	REQUIRE(p4.Y == Approx(-2.2));
	
	VerticalMirror(p4, Pointf{2, 2});
	REQUIRE(p4.X == Approx(5.2));
	REQUIRE(p4.Y == Approx(-2.2));
	
	HorizontalMirror(p4, Pointf{2, 2});
	REQUIRE(p4.X == Approx(5.2));
	REQUIRE(p4.Y == Approx(6.2));
	
}

TEST_CASE( "Size constructors", "[Size]" ) {
	Size  s1(2, 5);
	Size  s2(4);
	Size  s3(2.2, 5.2);
	Sizef s4(4.4);
	Sizef s5(2.2, 5.2);
	Size  s6;
	Sizef s7;
	Size  s8(Point(2, 5));
	Sizef s9(Pointf(2.2, 5.2));
	Point p1(2, 5);
	Pointf p2(2.2, 5.2);
	Size  s10;
	Sizef s11;
	Size  s12("2x5");
	Sizef s13("2.2x5.2");
	
	s6=s1;
	s7=s5;
	
	s10=p1;
	s11=p2;
	
	REQUIRE(s1.Width == 2);
	REQUIRE(s1.Height == 5);
	
	REQUIRE(s2.Width == 4);
	REQUIRE(s2.Height == 4);

	REQUIRE(s3.Width == 2);
	REQUIRE(s3.Height == 5);

	REQUIRE(s4.Width == Approx(4.4));
	REQUIRE(s4.Height == Approx(4.4));

	REQUIRE(s5.Width == Approx(2.2));
	REQUIRE(s5.Height == Approx(5.2));
	
	REQUIRE(s6.Width == 2);
	REQUIRE(s6.Height == 5);

	REQUIRE(s7.Width == Approx(2.2));
	REQUIRE(s7.Height == Approx(5.2));
	
	REQUIRE(s8.Width == 2);
	REQUIRE(s8.Height == 5);

	REQUIRE(s9.Width == Approx(2.2));
	REQUIRE(s9.Height == Approx(5.2));
	
	REQUIRE(s10.Width == 2);
	REQUIRE(s10.Height == 5);

	REQUIRE(s11.Width == Approx(2.2));
	REQUIRE(s11.Height == Approx(5.2));

	REQUIRE(s12.Width == 2);
	REQUIRE(s12.Height == 5);

	REQUIRE(s13.Width == Approx(2.2));
	REQUIRE(s13.Height == Approx(5.2));
	
	//REQUIRE( ((basic_Point<int>)s1) == Point(2, 5) );
	//REQUIRE( (Pointf)s2 == Pointf(4, 4) );
}

TEST_CASE( "Size comparison", "[Size]" ) {
	Size  s1(1, 2);
	Sizef s2(1.2f, 2.2f);
	Size  s3(1, 2);
	Sizef s4(1.2f, 2.2f);
	Size  s5(3, 4);
	Sizef s6(3.3f, 7.1f);

	REQUIRE( s1 == s3 );
	REQUIRE( s2 == s4 );

	REQUIRE_FALSE( s1 == s5 );
	REQUIRE_FALSE( s2 == s6 );

	REQUIRE_FALSE( s1 != s3 );
	REQUIRE_FALSE( s2 != s4 );

	REQUIRE( s1 != s5 );
	REQUIRE( s2 != s6 );

	REQUIRE( s1 == s1 );
	REQUIRE_FALSE( s1 != s1 );

	REQUIRE( s2 == s2 );
	REQUIRE_FALSE( s2 != s2 );
}

TEST_CASE( "Size arithmetic", "[Size]" ) {

	Size  s1(1, 2);
	Sizef s2(1.2f, 2.2f);
	Size  s3(5, 6);
	Sizef s4(5.5f, 6.8f);

	Size  s5;
	Sizef s6;

	s5=s1+s3;
	REQUIRE( s5 == Size(6,8) );

	s5=s1-s3;
	REQUIRE( s5 == Size(-4,-4) );

	s5=s1*2;
	REQUIRE( s5 == Size(2,4) );

	s5=s1/2;
	REQUIRE( s5 == Size(0,1) );

	s5=s1*1.5;
	REQUIRE( s5 == Size(1,3) );

	s5=s1/0.5;
	REQUIRE( s5 == Size(2,4) );

	s6=s2+s4;
	REQUIRE( s6.Width == Approx(6.7f) );
	REQUIRE( s6.Height == Approx(9.0f) );

	s6=s2-s4;
	REQUIRE( s6.Width == Approx(-4.3f) );
	REQUIRE( s6.Height == Approx(-4.6f) );

	s6=s2*2;
	REQUIRE( s6.Width == Approx(2.4f) );
	REQUIRE( s6.Height == Approx(4.4f) );

	s6=s2/2;
	REQUIRE( s6.Width == Approx(0.6f) );
	REQUIRE( s6.Height == Approx(1.1f) );

	s6=s2*1.5;
	REQUIRE( s6.Width == Approx(1.8f) );
	REQUIRE( s6.Height == Approx(3.3f) );

	s6=s2/0.5;
	REQUIRE( s6.Width == Approx(2.4f) );
	REQUIRE( s6.Height == Approx(4.4f) );



	s5=s1;
	s5-=s3;
	REQUIRE( s5 == Size(-4,-4) );
	//REQUIRE_FALSE( s5.IsValid() );

	s5=s1;
	s5*=2;
	REQUIRE( s5 == Size(2,4) );
	//REQUIRE( s5.IsValid() );

	s5=s1;
	s5/=2;
	REQUIRE( s5 == Size(0,1) );

	s5=s1;
	s5*=1.5;
	REQUIRE( s5 == Size(1,3) );

	s5=s1;
	s5/=0.5;
	REQUIRE( s5 == Size(2,4) );

	
	s6=s2;
	s6-=s4;
	REQUIRE( s6.Width == Approx(-4.3f) );
	REQUIRE( s6.Height == Approx(-4.6f) );

	s6=s2;
	s6*=2;
	REQUIRE( s6.Width == Approx(2.4f) );
	REQUIRE( s6.Height == Approx(4.4f) );

	s6=s2;
	s6/=2;
	REQUIRE( s6.Width == Approx(0.6f) );
	REQUIRE( s6.Height == Approx(1.1f) );

	s6=s2;
	s6*=1.5;
	REQUIRE( s6.Width == Approx(1.8f) );
	REQUIRE( s6.Height == Approx(3.3f) );

	s6=s2;
	s6/=0.5;
	REQUIRE( s6.Width == Approx(2.4f) );
	REQUIRE( s6.Height == Approx(4.4f) );

}

TEST_CASE( "Size geometric info", "[Size]" ) {
	Size  s1(1, 2);
	Sizef s2(1.2f, 2.2f);
	Size  s3(5, 6);
	Sizef s4(5.5f, 6.8f);

	REQUIRE( s1.Area() == 2 );
	REQUIRE( s2.Area() == Approx(2.64) );
	
	REQUIRE( s3.Cells() == 30 );
	REQUIRE( s4.Cells() == 30 );
}

TEST_CASE( "Size <-> string", "[Size]" ) {
	
	Size  s1("1,2");
	Sizef s2("1.2fx2.2f");
	Size  s3;
	Sizef s4;

	std::stringstream ss;
	auto makestream=[&](std::string s) -> std::stringstream & {
		s3 = {0,0};
		s4 = {0,0};
		
		ss.str(s);
		ss.clear();
		
		return ss;
	};
	
	auto resetstream=[&] {
		s3 = {0,0};
		s4 = {0,0};
		
		ss.str("");
		ss.clear();
	};
	
	
	makestream("1x2")>>s3;
	REQUIRE( s1 == s3 );
	
	makestream("1.2x2.2")>>s3;
	REQUIRE( s1 == s3 );
	
	makestream("-1.2x-2.2")>>s3;
	REQUIRE( s1 == s3*-1 );
	
	
	makestream("1x 2")>>s3;
	REQUIRE( s1 == s3 );
	
	makestream("1.2 x2.2")>>s3;
	REQUIRE( s1 == s3 );
	
	makestream("-1.2 x -2.2")>>s3;
	REQUIRE( s1 == s3*-1 );
	

	makestream("1 x2")>>s3;
	REQUIRE( s1 == s3 );
	
	makestream("1.2 x 2.2")>>s3;
	REQUIRE( s1 == s3 );
	
	makestream("-1.2x-2.2")>>s3;
	REQUIRE( s1 == s3*-1 );
	
	
	makestream("1x2 1.2x2.2")>>s3>>s4;
	REQUIRE( s1 == s3 );
	REQUIRE( s2 == s4 );
	
	makestream("-1.2x-2.2 -1.2x-2.2")>>s3>>s4;
	REQUIRE( s1 == s3*-1 );
	REQUIRE( s2 == s4*-1 );

	
	REQUIRE( std::string(s1) == "1x2" );
	
	//Its ok to have extra zeros at the end
	REQUIRE( Gorgon::String::Replace(std::string(s2),"0","") == "1.2x2.2" );
	
	REQUIRE( std::string(s1*-1) == "-1x-2" );
	
	
	resetstream();
	ss<<s1;
	REQUIRE( Gorgon::String::Replace(ss.str(), " ","") == "1x2" );

	resetstream();
	ss<<s2;
	REQUIRE( Gorgon::String::Replace(ss.str(), " ","") == "1.2x2.2" );
	
	resetstream();
	ss<<s1;
	makestream(ss.str())>>s3;
	REQUIRE( s1 == s3 );
	
	resetstream();
	ss<<s2;
	makestream(ss.str())>>s4;
	REQUIRE( s2 == s4 );
	
	REQUIRE( Gorgon::String::To<Size>("1x2") == s1 );
	REQUIRE( Gorgon::String::To<Size>("1.2x2.2") == s1 );
	REQUIRE( Gorgon::String::To<Sizef>("1.2x2.2") == s2 );
	
	REQUIRE( Gorgon::String::To<Size>("1 x2") == s1 );
	REQUIRE( Gorgon::String::To<Size>("1.2x 2.2") == s1 );
	REQUIRE( Gorgon::String::To<Sizef>("1.2 x 2.2") == s2 );
	
	REQUIRE( Size::Parse("3x5") == Size(3,5) );
	REQUIRE( Size::Parse("3 x5") == Size(3,5) );
	REQUIRE( Size::Parse("3x 5") == Size(3,5) );
	REQUIRE( Size::Parse("3.2x5.8") == Size(3,5) );
	REQUIRE( Size::Parse(" 3x  5 ") == Size(3,5) );
	
	REQUIRE_THROWS( Size::Parse("") );
	REQUIRE_THROWS( Size::Parse("3x") );
	REQUIRE_THROWS( Size::Parse("3xa") );
	REQUIRE_THROWS( Size::Parse("7ax5e") );
	REQUIRE_THROWS( Size::Parse("3,4") );
	REQUIRE_THROWS( Size::Parse("3,4)") );
	REQUIRE_THROWS( Size::Parse("(3,4)") );
}

TEST_CASE( "Size point interaction", "[Size][Point]" ) {
	Point  p1(10,20);
	Size   s1(2,5);
	Pointf p2(1.2, 2.2);
	Sizef  s2(2.2, 5.2);
	Pointf p3;
	Point  p4;
	
	REQUIRE( (p1*s1) == Point(20, 100) );
	REQUIRE( (p1/s1) == Point(5, 4) );
	REQUIRE( (p1*s2) == Pointf(22, 104) );
	p3=(p1/s2);
	REQUIRE( p3.X == Approx(4.5454) );
	REQUIRE( p3.Y == Approx(3.8461) );
	
	p3=(p2*s1);
	REQUIRE(  p3.X == Approx(2.4) );
	REQUIRE(  p3.Y == Approx(11) );
	
	p3=(p2/s1);
	REQUIRE(  p3.X == Approx(0.6) );
	REQUIRE(  p3.Y == Approx(0.44) );

	p3=(p2*s2);
	REQUIRE( p3.X == Approx(2.64) );
	REQUIRE( p3.Y == Approx(11.44) );

	p3=(p2/s2);
	REQUIRE( p3.X == Approx(0.54545) );
	REQUIRE( p3.Y == Approx(0.42308) );
}
	/*p4=p1;
	Scale(p4, s1);
	REQUIRE( p4 == Point(20, 100) );
	
	p4=p1;
	Scale(p4, s2);
	REQUIRE( p4 == Point(22, 103) ||  p4 == Point(22, 104) );
	
	p3=p2;
	Scale(p3, s1);
	REQUIRE(  p3.X == Approx(2.4) );
	REQUIRE(  p3.Y == Approx(11) );
	
	p3=p2;
	Scale(p3, s2);
	REQUIRE( p3.X == Approx(2.64) );
	REQUIRE( p3.Y == Approx(11.44) );
}
TEST_CASE( "Size geometric functions", "[Size]" ) {
	Size  s1(1, 2);
	Sizef s2(1.2f, 2.2f);
	
	Scale(s1, 2);
	REQUIRE( s1 == Size(2, 4) );
	
	Scale(s1, 0.25);
	REQUIRE( s1 == Size(0, 1) );
	
	Scale(s2, 2);
	REQUIRE( s2.Width == Approx(2.4) );
	REQUIRE( s2.Height == Approx(4.4) );
	
	Scale(s2, 0.25);
	REQUIRE( s2.Width == Approx(0.6) );
	REQUIRE( s2.Height == Approx(1.1) );
}

TEST_CASE( "Point3D") {
    Point3D p1(1, 2, 3);
    Point3D p2;
    
    auto similar = [](Point3D p1, Point3D p2) {
        return fabs(p1.X - p2.X) < 0.00001 && fabs(p1.Y - p2.Y) < 0.00001 && fabs(p1.Z - p2.Z) < 0.00001;
    };
    
    REQUIRE(p1.X == 1);
    REQUIRE(p1.Y == 2);
    REQUIRE(p1.Z == 3);
    
    p2.X = 4;
    p2.Y = 5;
    p2.Z = 6;
    
    REQUIRE(similar(p1 * 3, {3.f, 6.f, 9.f}));
    REQUIRE(similar(p1 / 2, {.5f, 1.f, 1.5f}));
    REQUIRE(similar(p1 + p2, {5.f, 7.f, 9.f}));
    REQUIRE(similar(p1 - p2, {-3.f, -3.f, -3.f}));
    REQUIRE(Approx(p1 * p2) == 32);
    REQUIRE(similar(p1.CrossProduct(p2), {-3.f, 6.f, -3.f}));
    REQUIRE(similar(p1.Normalize(), p1 / 3.741657));    
    REQUIRE(Approx(p1.Distance()) == 3.741657);
    REQUIRE(Approx(p1.ManhattanDistance()) == 6);
}

TEST_CASE( "Transform3D") {
    
    auto similar = [](Transform3D t1, Transform3D t2) {
        for(int i=0; i<16; i++) {
            if(fabs(t1.Data()[i] - t2.Data()[i]) > 0.00001)
                return false;
        }
        
        return true;
    };
   
    auto similarp = [](Point3D p1, Point3D p2) {
        return fabs(p1.X - p2.X) < 0.00001 && fabs(p1.Y - p2.Y) < 0.00001 && fabs(p1.Z - p2.Z) < 0.00001;
    };
    
    Transform3D t1, t2;
    Point3D p1(1, 2, 3);

    //check if identity is loaded
    for(int i=0; i<4; i++) {
        for(int j=0; j<4; j++) {
            if(i == j)
                REQUIRE(t1(i, j) == 1);
            else
                REQUIRE(t1(i, j) == 0);
        }
    }
    
    REQUIRE(similarp(t1 * p1, p1));
    
    t1.Translate({1, 2, 3});
    t2(0, 3) = 1;
    t2(1, 3) = 2;
    t2(2, 3) = 3;
    
    REQUIRE(similar(t1, t2));
    
    glutil::MatrixStack m;
    m.SetIdentity();
    m.Translate(1, 2, 3);
    m.Rotate({0,0,1}, Gorgon::PI/2);
    m.Translate(1, 2, 3);
    for(int i=0; i<4; i++) {
        for(int j=0; j<4; j++) {
            std::cout<<std::fixed<<std::setprecision(1)<<std::setw(5)<<m.Top().operator[](j)[i]<<" ";
        }
        std::cout<<std::endl;
    }
    
    glm::vec4 v(3, 3, 3, 1);
    v = m.Top() * v;
    
    std::cout<<v.x<<" "<<v.y<<" "<<v.z<<std::endl;
    
    
    t2 = {{0,-1,0,1}, {1,0,0,2},{0,0,1,3},{0,0,0,1}};
    t1.Rotate(0, 0, Gorgon::PI/2);
    REQUIRE(similar(t1, t2));
    
    t2 = {};
    t2(0, 3) = 1;
    t2(1, 3) = 2;
    t2(2, 3) = 3;
    t1 *= t2;
    
    t2 = {{0,-1,0,-1}, {1,0,0,3},{0,0,1,6},{0,0,0,1}};
    REQUIRE(similar(t1, t2));
    
    t2 = {{1, 1, 1, 0}, {0,0,0,0},{0,0,0,0},{0,0,0,1}};
    t2.Transpose();
    REQUIRE(similar(t2, {{1,0,0,0}, {1,0,0,0}, {1,0,0,0}, {0,0,0,1}}));
    
    p1 = {3, 3, 3};
    p1 = t1 * p1;
    
    REQUIRE(similarp(p1, {-4, 6, 9}));
    
    t1 = {};
    t1.Scale(1, 2, 3);
    p1 = {3, 3, 3};
    p1 = t1 * p1;
    
    REQUIRE(similarp(p1, {3, 6, 9}));
}


TEST_CASE( "Bounds constructors", "[Bounds]" ) {
	Bounds  b1(1.2, 2.2);
	Boundsf b2(1.2, 2.2);
	Bounds  b3(b2);	Boundsf b4(b3);
	Bounds  b5;
	Boundsf b6;
	Boundsf b7;
	Bounds  b8("1, 2");
	Boundsf b9("1.2, 2.2");
	
	
}

*/