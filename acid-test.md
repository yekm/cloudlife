# AcidWarp llm rewrite notes

| name | money per 1M token | total cost | total time taken | overall result |
| ---- | ------------------ | ---------- | ---------------- | -------------- |
| GPT 5.6 terra pro | 666 | 64  | < 4 min  | no fades. palette used as is. some modes look teared |
| qwen 3.8 max [^1] | 634 | 134 | > 60 min | fade-ins and outs. colors look closer to original. |
| GPT 5.6 luna pro | 64 | 30  | < 5 min  | some modes look the same |
| MiniMax M3 [^2] | 103 | 60  | < 10 min  | scale slider do nothing |


### prompt
```
explore project. focus on EaselCompute easel and PlasmaCompute art.
thoroughly analyze original AcidWarp art. make new AcidWarp-llm-name art based on it,
but make it more efficient: use compute shaders and colormap-shaders. dont try to run binary, just write the code.
```

### notes
sometimes opencode client running on macos: no way to check compute shaders.
promt is changed accordingly in this case.


[^1]: read what gpt had done.
prompted to skip glsl build validation after 20 minutes of slop.

[^2]: too much `Now let me double-check the implementation by reading it back:`.
