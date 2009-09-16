import urllib

class Values(list):
    def __init__(self):
        list.__init__(self)
        self.add_value("monochrome", "0")
        self.add_value("color", "1")
        self.add_value("jp46", "2")

    def add_value(self, name, value):
        self.append({"name": name, "value": value})

class ElphelColorStatus:
    def __init__(self, ip):
        self.ip = ip
        self.pattern = "<td>COLOR</td><td style='text-align:right'>"
        self.url = 'http://%s/parsedit.php?COLOR' %ip
        self.values = Values()

    def get_image_mode(self):
        u = urllib.urlopen(self.url)
        data = u.readlines()
        for line in data:
            if line.find(self.pattern) != -1:
                pos = line.find(self.pattern)+len(self.pattern)
                param_value = line[pos:pos+1]
                for value in self.values:
                    if value["value"] == param_value:
                        print "Elphel %s is in %s compression mode" %(self.ip, value["name"])
                        return value["name"]

if __name__ == "__main__":
    t = ElphelColorStatus("192.168.1.9")
