import numpy as np
import pandas as pd
import uproot

df1 = pd.DataFrame({
    "var1" : np.random.normal(0, 1, 1000),
    "var2" : np.random.normal(1, 2, 1000),
    "var3" : np.random.normal(2, 3, 1000),
    "var4" : np.random.normal(3, 4, 1000),
    "var5" : np.random.normal(4, 5, 1000),
})

df2 = pd.DataFrame({
    "var1" : np.random.normal(0, 1, 1000),
    "var2" : np.random.normal(1, 2, 1000),
    "var3" : np.random.normal(2, 3, 1000),
    "var4" : np.random.normal(3, 4, 1000),
})

with uproot.recreate("build/ntuple.root") as F:
    F["DecayTree"] = df1
    F["tree_1"] = df1
    F["main_folder/tree"] = df1
    F["tree_2"] = df1
    F["tree_3"] = df1
    F["utility/subfolder/particles"] = df2
    F["Words"] = "Hello World!"

