"""
	parse tubes.tmx and produces the right tilemaps/defines ...
"""
import xml.etree.ElementTree as ET

xml = ET.parse('tubes.tmx')

tiles = [] # 9-tuple index of tiles 
names = {} # name : id in tiles
transitions = [] # empty tile, source_dir, dest_dir, start_anim_tile (+0..7), final_anim_tile 
chars = {} # char id : id in tiles

def add_tile(elt) :
	"add 9 tilemap to tiles if not exists, adds if not ; return id"
	data = elt.find('data')
	assert data.get('encoding')=='csv'
	tmap = tuple(int(x) for x in data.text.split(','))

	try : 
		return tiles.index(tmap) # already exist
	except ValueError : 
		x=len(tiles)
		tiles.append(tmap)
		return x

def byname(l) : 
	return sorted(l, key = lambda x:x.get('name'))

# standard tiles or cross state. this works because similar tilemaps will be deduplicated
for name in ('ne','ns','se','sw','ew','nw','ew_cross','ns_cross',
	'ew_cross_full','ns_cross_full',
	'we_source','ew_fixed','ns_fixed') : 
	grp=xml.find("./group[@name='%s']"%name)
	
	start_elt = grp.find('./layer[@name="empty"]')
	if start_elt != None : # unique element empty
		start=add_tile(start_elt)
	else : 
		# start animation
		start=len(tiles)
		for frame in byname(grp.find('./group[@name="empty"]')) : 
			add_tile(frame)


	# FIXME build animations by progr
	# anim 0 : same direction as the name
	anim0_id = len(tiles)
	for frame in byname(grp.find("./group[@name='anim0']")) : 
		add_tile(frame)

	# anim 1 : reverse (might not exist)
	anim1_elt = grp.find("./group[@name='anim1']")
	if anim1_elt!=None : 
		anim1_id = len(tiles)
		for frame in byname(anim1_elt) : 
			add_tile(frame)
	else : 
		anim1_id=0

	# end frame
	end = add_tile(grp.find('./layer[@name="full"]'))

	names[name]=start
	transitions.append((start, name[0], name[1], anim0_id, end))
	
	if name=='we_source' : # record transition 
		source_transition = len(transitions)-1

	if anim1_id : 
		transitions.append((start, name[1], name[0], anim1_id, end))

# single animations
for name in 'blocked','explo','empty' : 
	names[name]=len(tiles)
	for frame in byname(xml.find("./group[@name='%s']"%name)) :
		add_tile(frame)

# big letters and numbers
for char in xml.find("./group[@name='chars']") : 
	chr=char.get('name')
	chars[chr] = add_tile(char)

# read level tilemap - translated to tile ids + extract speeds
levels={} # id: map, parameters
xml = ET.parse('levels.tmx')
for l in xml.findall('layer') : 
	layer_id = int(l.get('name').split('level')[1])
	dat=l.find('data')
	assert dat.get('encoding')=='csv'
	assert l.get('width')=='10'
	assert l.get('height')=='7'

	tmap = tuple(int(i) for i in dat.text.replace('\n','').split(','))

	d={}
	for elt in l.findall('properties/property') :
		d[elt.get('name')] = int(elt.get('value'))

	levels[layer_id]=tmap,(d.get('speed',30),d.get('start',10),d.get('steps',10))
levels = [levels[lid] for lid in sorted(levels)]

# output ============================================================================

def nmstr(x) : return 'TUBE_'+x.upper() # name to C const

print '// generated file from mk_tubes.py - do not edit'
print

for nm,id in names.items() : 
	print "#define %s %d"%(nmstr(nm),id)
print


print "#define NB_TRANSITIONS",len(transitions)
print "const struct FlowTransition flow_transitions[NB_TRANSITIONS] = {"
for t in transitions : 
	# find the source element name
	name = nmstr(next(nm for nm, id in names.items() if id == t[0]))
	print "    {.start_frame=%20s, .src_dir=C_%s, .dest_dir=C_%s, .start_anim=%3d, .end_anim=%3d},"%(name, t[1].upper(),t[2].upper(),t[3],t[4])
print "};"
print

print "#define SOURCE_TRANSITION",source_transition
print 

print "const uint8_t chars_maps[] = {"

for c,i in chars.items() : 
	print "  [%2d]=%d, // %s"%(ord(c)-32,i,c if c != ' ' else '<space>')
print '};'
print

# export tilemaps
print "const uint8_t tiles[][9] = {"
for tile in tiles : 
	print '    {%d,%d,%d,%d,%d,%d,%d,%d,%d},'%tile
print '};'


# id to tilename - FIXME : read from xml
id2tile = {
	-1 : 'empty',

	112: 'we_source',
	120: 'blocked',
}

print "#define NB_LEVELS",len(levels)
print "const uint8_t level_map[NB_LEVELS][70]={"
for l in levels :
	dat= l[0]	
	print "    {%s},"%','.join(str(names[id2tile[x-1]]) for x in dat)
print "};"

print "const struct LevelProperty level_properties[NB_LEVELS]={"
for l in levels : print "    {.flow_speed=%d, .flow_start=%d, .steps_needed=%d},"%l[1]
print "};"
