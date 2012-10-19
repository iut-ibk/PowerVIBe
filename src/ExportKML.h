//
//  exportkml.h
//  DynaMind-ToolBox
//
//  Created by Christian Urich on 10/19/12.
//
//

#ifndef __DynaMind_ToolBox__exportkml__
#define __DynaMind_ToolBox__exportkml__

#include <iostream>
#include <dm.h>

class DM_HELPER_DLL_EXPORT ExportKML : public DM::Module
{
    DM_DECLARE_NODE(ExportKML)
private:
    void concertCoordinates(double &x, double &y);
public:
    ExportKML();
    void run();
};

#endif /* defined(__DynaMind_ToolBox__exportkml__) */
