### Test readme  
To tets on provided examples, first start the validator:  
```./validator < [test-name]_validator.in ```  

and then start one (or more) testers:
```./tester < [same-test-name]_tester_[tester-type].in ```  
or  
```./tester < [same-test-name]_tester.in ```  
(some tests are prepared to use with multiple testers running).  

After programs finish running, compare results with corresponding `*.out` files for the test case.  
