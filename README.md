# LaptopFactory
 a multi-threading application that simulates the behavior of a
laptop manufacturing factory. The system behaves as follows:
• There are 10 manufacturing lines inside the factory. Each line is composed of 10
technical employees each executing a step in the laptop manufacturing process. Each
step might take a certain amount of time but is bound in a range between a min
value and a max value.
• Steps 1 to 5 have to happen in order. Steps 6 to 10 can happen in any order.
• Once the above 10 steps are executed, the laptop manufacturing process is over and
the technical employee that executed the last step should put the laptop in a carton
box placed at the end of the line.
• Once 10 laptops are placed in a carton box, a storage employee is responsible to
collect the filled carton box and place it in the storage room. The storage employee
will become absent for a certain user-defined period of time until he finishes his task.
• The storage area can contain a maximum number of manufactured laptops (user-
defined). If the storage room capacity goes beyond the maximum threshold, the
technical employees should stop manufacturing more laptops until the storage room
capacity goes below a minimum threshold value (user-defined).
• The factory employs as well several loading employees whose job is to load trucks
with the laptop cartons in the storage room. Assume each truck can hold a user-
defined number of cartons in each trip. Of course, each truck becomes unavailable
for a user-defined period when shipping the laptops.
• The factory has a CEO with salary salaryceo and an HR manager with salary
salaryhr. Assume also all technical employees have the same salary salaryt, all
storage employees have the same salary salarys, all loading employees have the
same salary salaryl and all truck drivers have the same salary salaryu. Assume
as well that any extra employee you might need to hire to do the simulation has a
salary salarya (e.g. storage room keeper, security personnel, etc). Thus, the only
expense the factory has is the employees’ salaries.
• Assume also that each laptop’s cost is costfab and that the laptops are sold each at
pricesell.
• If the factory profit (what the factory gains minus all expenses) drops below a
user-defined threshold, the HR manager informs the CEO to cut on expenses by
suspending some employees. The CEO will suspend the employees belonging to a
whole manufacturing line in 1 shot. If the factory gains increase beyond a certain
threshold afterwards, the HR manager informs the CEO to cancel the employees’
suspension. Obviously, if the factory gains decrease again, the HR manager might
ask the CEO to expand the suspension plan to include more manufacturing lines
