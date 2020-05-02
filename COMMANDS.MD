Communication Protocol
======================

The system acts as a DALI master so it can send/receive
DALI commands to/from its slaves according to an host system's
commands.

The communication port is a standard UART at 115200 8N1.

Each command is syncronous, that is for each command we should get an
answer before the cmd_answer_timeout time expires. If we get no
answers before the cmd_answer_timeout time and after cmd_answer_retries
retries the system must be considered faulty.

By default for all commands the cmd_answer_timeout
is set to 200ms and cmd_answer_retries is set to 5.

All commands/answers are strings written in ASCII terminated by
'\n' (max lenght is 128 bytes).

For telnet compatibility the sequence "\r\n" is accepted as "\n".

All answers are in this form:

	TXXX..XXX\n

where:

 - T: the type of answer, where 'O' is for ok and 'E' is for error

 - X: an optional argument byte 

Where values are expecting multiple digits, include any leading ZEROs to pad the value. 

**Example**

* 09 for a short address instead of 9

* 002 for a arc value instead of 2

Addresses
---------------

* **Slave addresses are in decimal form and range between 0 - 63**

* **Group addresses are in decimal form and range between 0 - 15**



Remap Commands:
---------------

- remap all: start remapping all slaves on bus.

        ra\n

- remap unknown: start remapping slaves which do not have assigned short addresses.

        ru\n

- remap start: start remapping all slaves starting from address XX

        rsXX\n

- remap move: change slave address from short address XX to short address YY

        rmXXYY\n

- remap progress: check on remap progress

        rp\n

	The answer byte is 0x00 for "yes" and a value from 1 to 100 for "no" representing the percentage
	of accomplished job.

	For this command the answer timeout is 500ms.

- remap abort: abort remap process

        rA\n


Configuration Commands
-----------------------

*Light Levels will range between 0 (Off) to 254 (Full On)*

- configure minimum level value YYY at address XX

        cmXXYYY\n
        

- configure maximum level value YYY at address XX

        cxXXYYY\n

- configure system fail level value YYY at address XX

        cfXXYYY\n

- configure power on level value YYY at address XX

        cpXXYYY\n

- configure fade time value YYY at address XX

        ctXXYYY\n

  **Fade Time**
  | Value	| Time (Sec)|
  |-------|:---------:|
  |	000	|	< 0.7	|
  |	001	|	0.7		|
  |	002	|	1		|
  |	003	|	1.4		|
  |	004	|	2		|
  |	005	|	2.8		|
  |	006	|	4		|
  |	007	|	5.6		|
  |	008	|	8		|
  |	009	|	11.3	|
  |	010	|	16		|
  |	011	|	22.6	|
  |	012	|	32		|
  |	013	|	45.2	|
  |	014	|	64		|
  |	015	|	90.5	|

- configure fade rate value YYY at address XX

        crXXYYY\n

  **Fade Rate**
  | Value	|Steps (Sec)|
  |-------|:---------:|
  |	000	|	358		|
  |	001	|	253		|
  |	002	|	179		|
  |	003	|	126		|
  |	004	|	89.4	|
  |	005	|	63.2	|
  |	006	|	44.7	|
  |	007	|	31.6	|
  |	008	|	22.4	|
  |	009	|	11.3	|
  |	010	|	15.8	|
  |	011	|	11.2	|
  |	012	|	7.9		|
  |	013	|	5.6		|
  |	014	|	3.9		|
  |	015	|	2.8	|

Device Commands:
----------------

- device ON at address XX

        d1XX\n

	Turn on light on the slave device.

- device OFF at address XX

        d0XX\n

	Turn off light on the slave device.

- device SET ARC at value YYY address XX

        daXXYYY\n

	Set the arc light at specified power on the slave device.

- device INFO at address XX

	diXX\n
	
	Get device info
	
- device INFO for all addresses

	dia\n
	
	Get device info for all addresses

Group Device Commands:
----------------------

- group ON for group XX

        g1XX\n

	Turn on light (or whatever) on all slave devices in group XX.

- group OFF for group XX

        g0XX\n

	Turn off light (or whatever) on all slave devices in group XX.

- group SET ARC at value YYY for group XX

        gaXXYYY\n

	Set the arc light (or whatever) at specified power on all slave devices in a group.


Bus Commands:
-------------

- bus set all ON

        b1\n 

	Turn on light (or whatever) on all slaves on bus.


- bus set all OFF

        b0\n 

	Turn off light (or whatever) on all slaves on bus.


- bus set arc at YYY on all slaves on bus

        baYYY\n

	Set the arc light (or whatever) at specified power on all slave devices on bus.

- bus list of all slaves from memory

        bl\n

	The answer is a 65 byte long string where:

	* the first element is the return code,

	* the i-th element is not 0 if the i-th slave device is present on the bus. 

- bus scan of all slaves

        bs\n 

	Initiates a scan of all slaves on the bus
	
	The answer is a 65 bytes long string where:

	* the first element is the return code,

	* the i-th element is not 0 if the i-th slave device is present on the bus.

Error codes
-----------

Here a list of the error codes whose each DALI command can return.

=== Invalid command (0x01)

The submitted command is not valid.

=== Unsupported command (0x02)

The submitted command is not been implemented yet.

=== Bus busy (0x03)

The submitted command cannot be executed because the master is busy.

=== Invalid BUS ID (0x20)

The submitted bus ID is not valid.

=== Invalid DEV address (0x30)

Does not exist any device at this DEV address.

=== Timeout (0x90)

Returned each time a timeout expires.
