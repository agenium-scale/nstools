```c++
size_t len = size_t(nsimd::len(f32()));
for (size_t i = 0; i < N; i += len) {
  // auto is nsimd::pack<f32>
  auto v0 = nsimd::loada<nsimd::pack<f32> >(&in0[i]);
  auto v1 = nsimd::loada<nsimd::pack<f32> >(&in1[i]);
  auto r = v0 + v1;
  nsimd::storea(&out[i], r);
}
```
