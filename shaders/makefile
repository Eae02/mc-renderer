SC=glslangValidator
FLAGS=-V

OUT_PATH=../res/spv

SHADERS=block.vs block.fs block-shadow.vs block-shadow.gs block-shadow.fs debug.vs debug.fs sky.vs sky.fs ui.vs ui.fs \
godrays.vs godrays-gen.fs godrays-hblur.fs water.vs water.fs water-tess.vs water.tcs water.tes water-post.fs caustics.cs post.vs post.fs
HEADER_FILES=$(wildcard inc/*.glh)

all: $(addsuffix .spv,$(addprefix $(OUT_PATH)/,$(SHADERS)))

$(OUT_PATH)/%.vs.spv : %.vs.glsl $(HEADER_FILES)
	$(SC) $(FLAGS) -S vert $< -o $@

$(OUT_PATH)/%.fs.spv : %.fs.glsl $(HEADER_FILES)
	$(SC) $(FLAGS) -S frag $< -o $@
	
$(OUT_PATH)/%.gs.spv : %.gs.glsl $(HEADER_FILES)
	$(SC) $(FLAGS) -S geom $< -o $@

$(OUT_PATH)/%.tcs.spv : %.tcs.glsl $(HEADER_FILES)
	$(SC) $(FLAGS) -S tesc $< -o $@

$(OUT_PATH)/%.tes.spv : %.tes.glsl $(HEADER_FILES)
	$(SC) $(FLAGS) -S tese $< -o $@

$(OUT_PATH)/%.cs.spv : %.cs.glsl $(HEADER_FILES)
	$(SC) $(FLAGS) -S comp $< -o $@
