@echo off 
FOR /L %%i IN (4,1,10) DO (
  C:\Python38\python.exe .\Assignment1.py %%i DFS
)
FOR /L %%i IN (4,1,10) DO (
  C:\Python38\python.exe .\Assignment1.py %%i BFS
)