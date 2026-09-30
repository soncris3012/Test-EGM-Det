#include "../src/evaluator.h"
#include <QCoreApplication>
#include <cmath>
#include <iostream>

int main(int argc,char**argv){QCoreApplication app(argc,argv);OrientedBox a{"car",{50,50},{40,20},30,1,true,"1"};if(std::abs(Evaluator::rotatedIoU(a,a)-1)>1e-6){std::cerr<<"identical IoU failed\n";return 1;}auto b=a;b.center={500,500};if(Evaluator::rotatedIoU(a,b)!=0){std::cerr<<"disjoint IoU failed\n";return 2;}Evaluator e;e.addGroundTruth(a);a.groundTruth=false;a.confidence=.9;e.addDetection(a);auto r=e.evaluate();if(r.rows.size()!=2||r.rows[0].ap50<99.9||r.rows[0].recall<99.9){std::cerr<<"perfect AP failed\n";return 3;}std::cout<<"All evaluator tests passed\n";return 0;}
