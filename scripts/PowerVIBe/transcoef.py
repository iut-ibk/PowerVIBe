
from numpy import genfromtxt
""" Class to load the get transfer coefficients from a csv file.
    Strucutre of the CSV file:
    before	location	type	building_standard	ceiling_cellar	ceiling_floor	wall_outside	ceiling_roof	window	g	front_door
    1600	Tirol       0       0                   1.25                0.75            1.55            1.3            2.5	0.67	2.5
    """
class HeatCoefficients:
    def __init__(self, filename):
        self.db = genfromtxt(filename, dtype=None, delimiter=',',names=True)
    
    def buildingPeriode(self, periods, year_built):
        print periods
        print year_built
        prev_p = periods[0]
        for p in periods:
            if p > year_built:
                return prev_p
            prev_p = p
        return periods[len(periods)-1]

    def searchDB(self, periode, type, location,  building_standard):
        
        ress = [{"ceiling_cellar": item["ceiling_cellar"],
                "ceiling_floor": item["ceiling_floor"],
                "wall_outside": item["wall_outside"],
                "ceiling_roof": item["ceiling_roof"],
                "front_door": item["front_door"],
                "window": item["window"]} for item in self.db if \
            item['before'] == periode and item['type'] == type and item['building_standard'] == building_standard and item['location'] == location]
        return ress
    
    """ Returns the heat ransfer coefficient in W/m2K for:
        - ceiling_cellar
        - ceiling_floor
        - wall_outside
        - ceiling_roof
        - window
        - front_door
        """
    def getCoefficients(self, part ,year_built, type="", location='Tirol',  building_standard=""):
        
        periode = self.buildingPeriode(self.db['before'],year_built)
        
        ress = self.searchDB(periode, type, location,  building_standard)
        if len(ress) > 0:
            print ress
            print part
            return ress[0][part]
    
        ress = self.searchDB(periode, type, location,  "")
        if len(ress) > 0:
            print ress
            print part
            return ress[0][part]
    
        ress = self.searchDB(periode, "", location,  "")
        if len(ress) > 0:
            print ress
            print part
            return ress[0][part]
        return ress


HeatCoefficients("/Users/christianurich/Documents/DynaMind-ToolBox/PowerVIBe/data/TransmissionCoefficient/default_transmission_coefficients_tirol.csv")

