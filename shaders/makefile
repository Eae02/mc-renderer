SC=glslangValidator
FLAGS=-V

OUT_PATH=../res/spv

SHADERS=block.vs block.fs post.cs
HEADER_FILES=$(wildcard inc/*.glh)

all: $(addsuffix .spv,$(addprefix $(OUT_PATH)/,$(SHADERS)))

$(OUT_PATH)/%.vs.spv : %.vs.glsl $(HEADER_FILES)
	$(SC) $(FLAGS) -S vert $< -o $@

$(OUT_PATH)/%.fs.spv : %.fs.glsl $(HEADER_FILES)
	$(SC) $(FLAGS) -S frag $< -o $@

$(OUT_PATH)/%.cs.spv : %.cs.glsl $(HEADER_FILES)
	$(SC) $(FLAGS) -S comp $< -o $@
