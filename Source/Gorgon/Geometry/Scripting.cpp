#include "Point.h"
#include "Bounds.h"
#include "../Scripting/Embedding.h"
#include "../Scripting/Reflection.h"
#include "../Scripting.h"
#include "Margin.h"

namespace Gorgon :: Geometry {



	Scripting::Library LibGeometry("Geometry",
								   "Data types under geometry module and their member functions and operators");

    void InitializeScripting() { 
		if(LibGeometry.Members.GetCount()) return;

		auto point = new Scripting::MappedValueType<Point>(
			"Point",
			"Basic class for representing a point in space."
			"The class is composed of two fields, x and y of type int."
			"See documentation for complete list of overloaded operators and member functions"
		);

		point->AddMembers({
			new Scripting::MappedOperator(
				"+",
				"Adds two point type to each other, returns a new resultant point",
				point, point, point, 
				[] (Point lhs, Point rhs) { return lhs + rhs; }
			),
            
            new Scripting::MappedOperator(
                "-",
                "Subtracts one point from another, returns a new resultant point",
                point,point,point,
                [](Point lhs, Point rhs){return lhs - rhs;}
            ),
            
            new Scripting::MappedOperator(
                "*",
                "Multiplies the point by a float value, basically scale function",
                point, point,Scripting::Types::Float(),
                [](Point lhs, Float rhs){return lhs * rhs;}
            ),
            
            new Scripting::MappedOperator(
                "/",
                "Divides the point by a float value, basically scale function",
                point,point,Scripting::Types::Float(),
                [](Point lhs, Float rhs){return lhs / rhs;}
            ),
            
            new Scripting::MappedOperator(
                "!=",
                "Checks if the curent point is not equal to the second",
                point,Scripting::Types::Bool(),point,
                [](Point lhs, Point rhs){return lhs != rhs;}
            ),
            
            new Scripting::MappedOperator(
                "=",
                "Checks if the current point is equal to the second point",
                point,Scripting::Types::Bool(),point,
                [](Point lhs, Point rhs){return lhs == rhs;}
            ),
            
            new Scripting::Function(
                "+=",
                "Adds a point from the left hand side to the right hand side",point,
                {
                    Scripting::MapFunction(
                        [](Point &lhs, Point rhs){return lhs += rhs;},point,
                        {
                            Scripting::Parameter("Point", "The float that is added to the first point and returns the result.",point)
                        }
                    )
                }
            ),
            
            new Scripting::Function(
                "-=",
                "Subtracts the point on the left hand side from the right hand side",point,
                {
                    Scripting::MapFunction(
                        [](Point &lhs, Point rhs){return lhs -= rhs;},point,
                        {
                            Scripting::Parameter("Point", "The point that is subtracted from the second point and returns the result.",point)
                        }
                    )
                }
            ),
            
            new Scripting::Function(
                "*=",
                "Multiplies the point by a float value and returns the result, basically scale function",point,
                {
                    Scripting::MapFunction(
                        [](Point &lhs, Float rhs){return lhs *= rhs;},point,
                        {
                            Scripting::Parameter("Float", "The float that is multiplied to the first point and returns the result.",Scripting::Types::Float())
                        }
                    )
                }
            ),
            
            new Scripting::Function(
                "/=",
                "Divides the point by a float value and returns the result, basically scale function",point,
                {
                    Scripting::MapFunction(
                        [](Point &lhs, Float rhs){return lhs /= rhs;},point,
                        {
                            Scripting::Parameter("Float", "The float that divides from the first point and returns the result.",Scripting::Types::Float())
                        }
                    )
                }
            ),

			new Scripting::Function("Distance",
				"Returns the distance between two Points", point,
				{
					Scripting::MapFunction(
						[] (Point owner, Point other) { 
							return owner.Distance(other); 
						}, Scripting::Types::Float(),
						{
							Scripting::Parameter("Point", "Another point to calculate distance in between", point)
						},
						Scripting::ConstTag
					),
					Scripting::MapFunction(
						[] (Point owner) { 
							return owner.Distance(); 
						}, Scripting::Types::Float(),
						{ },
						Scripting::ConstTag
					),
				}
			),
            
            new Scripting::Function("Slope",
                "Returns the slope of the point (Gradient)", point,
                {
                    Scripting::MapFunction(
                        [](Point owner, Point other){
                            return owner.Slope(other);
                        },Scripting::Types::Float(),
                        {
                            Scripting::Parameter("Point", "Another point to calculate the slope between",point)
                        },
                        Scripting::ConstTag
                    ),
                    Scripting::MapFunction(
                        [](Point owner){
                            return owner.Slope();
                        },Scripting::Types::Float(),
                        { },
                        Scripting::ConstTag
                    )
                }
            ),
            
            new Scripting::Function("Angle",
                "Returns the angle formed from the origin to this point", point,
                {
                    Scripting::MapFunction(
                        [](Point owner, Point other){
                            return owner.Angle(other);
                        },Scripting::Types::Float(),
                        {
                            Scripting::Parameter("Point", "Another point to calculate the angle between",point)
                        },
                        Scripting::ConstTag
                    ),
                    Scripting::MapFunction(
                        [](Point owner){
                            return owner.Angle();
                        },Scripting::Types::Float(),
                        { },
                        Scripting::ConstTag
                    )
                }
            ),
            
            new Scripting::Function("Compare",
                "Returns a boolean value after comparing two points against one another",point,
                {
                    Scripting::MapFunction(
                        [](Point owner, Point other){
                            return owner.Compare(other);
                        },Scripting::Types::Bool(),
                        {
                            Scripting::Parameter("Point", "Comparing a point against another",point)
                        },
                        Scripting::ConstTag
                    )
                }
            ),
            
            new Scripting::Function("ManhattanDistance",
                "Returns the Manhattan Distance between two points",point,
                {
                    Scripting::MapFunction(
                        [](Point owner, Point other){
                            return owner.ManhattanDistance(other);
                        },Scripting::Types::Int(),
                        {
                            Scripting::Parameter("Point","Another point to calculate the Manhattan Distance from",point)
                        },
                        Scripting::ConstTag
                    ),
                    Scripting::MapFunction(
                        [](Point owner){
                            return owner.ManhattanDistance();
                        },Scripting::Types::Int(),
                        { },
                        Scripting::ConstTag
                    )
                }
            ),
            
            new Scripting::Function("Scale",
                "scales the point by the factor provided",point,
                {
                    Scripting::MapFunction(
                        static_cast<void(*)(Point &, const Float &)>(Scale),NULL,
                        {
                            Scripting::Parameter("Float","What to scale the point by",Scripting::Types::Float())
                        }
                    )
                }
            ),
            
            new Scripting::Function("Move",
                "Moves this point to the given coordinate",point,
                {
                    Scripting::MapFunction(
                        [](Point *owner, int x, int y){
                            Point temp(0,0);
                            temp.Move(x,y);
                            *owner = temp;
                        },NULL,
                        {
                            Scripting::Parameter("Int","The new X coordinate of the point",Scripting::Types::Int()),
                            Scripting::Parameter("Int","The new Y coordinate of the point",Scripting::Types::Int())
                        }
                    ),
                    Scripting::MapFunction(
                        [](Point *owner, Point other){
                            *owner = other;
                        },NULL,
                        {
                            Scripting::Parameter("Point","The new point to at which the current point will be moved",point)
                        }
                    )
                }
            )
		});
        
		point->AddConstructors({
			Scripting::MapFunction([]{ return Point(0,0); }, point, {})
		});
		point->MapConstructor<int, int>({
			Scripting::Parameter("x", "X coordinate", Scripting::Types::Int()),
			Scripting::Parameter("x", "Y coordinate", Scripting::Types::Int())
		});

		point->AddMembers({
			new Scripting::MappedInstanceMember<Point, int>(&Point::X, "x", "X coordinate", Scripting::Types::Int()),
			new Scripting::MappedInstanceMember<Point, int>(&Point::Y, "y", "Y coordinate", Scripting::Types::Int())
		});
        
		LibGeometry.AddMember(point);
        
        
        
		auto pointf = new Scripting::MappedValueType<Pointf>(
			"Pointf",
			"Basic class for representing a point in space."
			"The class is composed of two fields, x and y of type float."
			"See documentation for complete list of overloaded operators and member functions"
		);

		LibGeometry.AddMember(pointf);
        
        
		auto size = new Scripting::MappedValueType<Size>(
			"Size",
			"Basic class for representing a size in space."
			"The class is composed of two fields, width and height of type int."
			"See documentation for complete list of overloaded operators and member functions"
		);

		LibGeometry.AddMember(size);
        
        
        
		auto sizef = new Scripting::MappedValueType<Sizef>(
			"Sizef",
			"Basic class for representing a size in space."
			"The class is composed of two fields, width and height of type float."
			"See documentation for complete list of overloaded operators and member functions"
		);

		LibGeometry.AddMember(sizef);
        

		auto bounds = new Scripting::MappedValueType<Bounds>(
			"Bounds",
			"Basic class for representing a bounds in space."
		);

		LibGeometry.AddMember(bounds);


		auto boundsf = new Scripting::MappedValueType<Boundsf>(
			"Boundsf",
			"Basic class for representing a bounds in space."
		);

		LibGeometry.AddMember(boundsf);


		auto margin = new Scripting::MappedValueType<Margin>(
			"Margin",
			"Basic class for representing a margin in space."
		);

		LibGeometry.AddMember(margin);


		auto marginf = new Scripting::MappedValueType<Marginf>(
			"Marginf",
			"Basic class for representing a bounds in margin."
		);

		LibGeometry.AddMember(marginf);
        
        
	}
}
