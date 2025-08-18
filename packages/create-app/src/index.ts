import { Command } from "commander";
import fs from "fs-extra";
import path from "path";
import updateCheck from "update-check";
import prompts from "prompts";
import { bold, cyan, green, red, yellow } from "picocolors";
import validateProjectName from "validate-npm-package-name";
import { spawn } from "child_process";
import {
  bindingVersion,
  cccCoreVersion,
  coreVersion,
  eslintPluginVersion,
  testtoolVersion,
} from "./config";

const packageJson = require(path.join(__dirname, "../package.json"));

let projectName = "";

const handleSigTerm = (): void => process.exit(0);

process.on("SIGINT", handleSigTerm);
process.on("SIGTERM", handleSigTerm);

const onPromptState = (state: any): void => {
  if (state.aborted) {
    // If we don't re-enable the terminal cursor before exiting
    // the program, the cursor will remain hidden
    process.stdout.write("\x1B[?25h");
    process.stdout.write("\n");
    process.exit(1);
  }
};

const program = new Command(packageJson.name)
  .version(
    packageJson.version,
    "-v, --version",
    `Output the current version of ${packageJson.name}.`,
  )
  .argument("[directory]")
  .usage("[directory] [options]")
  .helpOption("-h, --help", "Display this help message.")
  .option(
    "--skip-install",
    "Explicitly tell the CLI to skip installing packages.",
  )
  .option(
    "--use-npm",
    "Explicitly tell the CLI to generate npm packages(default pnpm)",
  )
  .action((name: string) => {
    // Commander does not implicitly support negated options. When they are used
    // by the user they will be interpreted as the positional argument (name) in
    // the action handler. See https://github.com/tj/commander.js/pull/1355
    if (name && !name.startsWith("--no-")) {
      projectName = name;
    }
  })
  .allowUnknownOption()
  .parse(process.argv);

const opts = program.opts();

function getPkgManager(): string {
  if (opts.useNpm) {
    return "npm";
  } else {
    return "pnpm";
  }
}

const packageManager = getPkgManager();

interface ValidationResult {
  valid: boolean;
  problems?: string[];
}

function validateNpmName(name: string): ValidationResult {
  const nameValidation = validateProjectName(name);
  if (nameValidation.validForNewPackages) {
    return { valid: true };
  }

  return {
    valid: false,
    problems: [
      ...(nameValidation.errors || []),
      ...(nameValidation.warnings || []),
    ],
  };
}

function isFolderEmpty(folderPath: string): boolean {
  const files = fs.readdirSync(folderPath);
  return files.length === 0;
}

export async function install(packageManager: string): Promise<void> {
  const args = ["install"];

  return new Promise((resolve, reject) => {
    const child = spawn(packageManager, args, {
      stdio: "inherit",
      env: {
        ...process.env,
        ADBLOCK: "1",
        // we set NODE_ENV to development as pnpm skips dev
        // dependencies when production
        NODE_ENV: "development",
        DISABLE_OPENCOLLECTIVE: "1",
      },
    });
    child.on("close", (code) => {
      if (code !== 0) {
        reject({ command: `${packageManager} ${args.join(" ")}` });
        return;
      }
      resolve();
    });
  });
}

function updateProjectPackage(projectPath: string) {
  const packageJsonPath = path.join(
    projectPath,
    "packages/on-chain-script/package.json",
  );
  let json: any = "";
  if (fs.pathExistsSync(packageJsonPath)) {
    try {
      json = fs.readJsonSync(packageJsonPath);
      json.name = projectName;
      json.devDependencies["@ckb-js-std/eslint-plugin"] = eslintPluginVersion;
      json.devDependencies["ckb-testtool"] = testtoolVersion;
      json.dependencies["@ckb-js-std/bindings"] = bindingVersion;
      json.dependencies["@ckb-js-std/core"] = coreVersion;
      fs.writeJsonSync(packageJsonPath, json, { spaces: 2 });
      console.log(green(`Updated ${projectName}/package.json.`));
    } catch (error: any) {
      console.error(
        red(
          `Failed to update packages/on-chain-script/package.json: ${error.message}`,
        ),
      );
      process.exit(1);
    }
  } else {
    console.error(
      red(
        `Could not find packages/on-chain-script/package.json in the template. Make sure your template includes a package.json.`,
      ),
    );
    process.exit(1);
  }
}

function updateTestPackage(projectPath: string) {
  const packageJsonPath = path.join(
    projectPath,
    "packages/on-chain-script-tests/package.json",
  );
  let json: any = "";
  if (fs.pathExistsSync(packageJsonPath)) {
    try {
      json = fs.readJsonSync(packageJsonPath);
      json.name = projectName + "-tests";
      json.devDependencies["ckb-testtool"] = testtoolVersion;
      json.devDependencies["@ckb-ccc/core"] = cccCoreVersion;

      fs.writeJsonSync(packageJsonPath, json, { spaces: 2 });
      console.log(green(`Updated ${projectName}/package.json.`));
    } catch (error: any) {
      console.error(
        red(
          `Failed to update packages/on-chain-script/package.json: ${error.message}`,
        ),
      );
      process.exit(1);
    }
  } else {
    console.error(
      red(
        `Could not find packages/on-chain-script/package.json in the template. Make sure your template includes a package.json.`,
      ),
    );
    process.exit(1);
  }
}

function updateRootPackage(projectPath: string) {
  const packageJsonPath = path.join(projectPath, "package.json");
  let json: any = "";
  if (fs.pathExistsSync(packageJsonPath)) {
    try {
      json = fs.readJsonSync(packageJsonPath);

      // Update scripts to use npm workspaces commands if using npm
      if (getPkgManager() === "npm") {
        json.scripts = {
          build: "npm run build --workspaces --if-present",
          clean: "npm run clean --workspaces --if-present",
          test: "npm run test --workspaces --if-present",
          format: "npm run format --workspaces --if-present",
          lint: "npm run lint --workspaces --if-present",
        };

        // Remove pnpm specific configuration
        if (json.pnpm) {
          delete json.pnpm;
        }
      }

      fs.writeJsonSync(packageJsonPath, json, { spaces: 2 });
      console.log(green(`Updated root package.json.`));
    } catch (error: any) {
      console.error(
        red(`Failed to update root package.json: ${error.message}`),
      );
      process.exit(1);
    }
  } else {
    console.error(
      red(
        `Could not find package.json in the template. Make sure your template includes a package.json.`,
      ),
    );
    process.exit(1);
  }
}

async function run(): Promise<void> {
  console.log();
  if (projectName && typeof projectName === "string") {
    projectName = projectName.trim();
  }

  if (!projectName) {
    const res = await prompts({
      onState: onPromptState,
      type: "text",
      name: "path",
      message: "What is your project named?",
      initial: "my-ckb-script",
      validate: (name) => {
        const validation = validateNpmName(path.basename(path.resolve(name)));
        if (validation.valid) {
          return true;
        }
        return (
          "Invalid project name: " +
          (validation.problems?.[0] || "Unknown validation error")
        );
      },
    });

    if (typeof res.path === "string") {
      projectName = res.path.trim();
    }
  }

  if (!projectName) {
    console.log(
      "\nPlease specify the project directory:\n" +
        `  ${cyan(program.name())} ${green("<project-directory>")}\n` +
        "For example:\n" +
        `  ${cyan(program.name())} ${green("my-ckb-script")}\n\n` +
        `Run ${cyan(`${program.name()} --help`)} to see all options.`,
    );
    process.exit(1);
  }

  const appPath = path.resolve(projectName);
  const appName = path.basename(appPath);

  const validation = validateNpmName(appName);
  if (!validation.valid) {
    console.error(
      `Could not create a project called ${red(
        `"${appName}"`,
      )} because of npm naming restrictions:`,
    );

    validation.problems?.forEach((p) =>
      console.error(`    ${red(bold("*"))} ${p}`),
    );
    process.exit(1);
  }

  if (fs.pathExistsSync(appPath) && !isFolderEmpty(appPath)) {
    console.error(
      `Could not create a project called ${red(
        `"${appName}"`,
      )} because a project with the same name already exists.`,
    );
    process.exit(1);
  }

  console.log(bold(`Using ${packageManager}.`));

  const templatePath = path.join(__dirname, `../templates`);
  if (!fs.pathExistsSync(templatePath)) {
    console.error(`Could not find a template`);
    console.error(`\n 😮‍💨 Project ${projectName} created failed!\n`);
    process.exit(1);
  }

  const originalDirectory = process.cwd();
  const projectPath = path.join(originalDirectory, projectName);

  fs.ensureDirSync(projectPath);
  fs.copySync(templatePath, projectPath);

  updateRootPackage(projectPath);
  updateProjectPackage(projectName);
  updateTestPackage(projectName);

  console.log(`\n🎉 Project ${projectName} created!\n`);

  if (opts.skipInstall) {
    console.log(
      "Skip install the dependencies, we suggest that you begin by typing:",
    );
    console.log();
    console.log(cyan("  cd"), projectName);
    console.log(`  ${cyan(`${packageManager} install`)}`);
    console.log();
  } else {
    console.log("\nInstalling dependencies:");
    console.log();
    console.log("Installing packages. This might take a couple of minutes.");
    console.log();

    process.chdir(appPath);
    await install(packageManager);
    console.log("Packages installed.");
    console.log();
  }

  // Create soft link for npm package manager to let ckb-debugger to find the path
  if (getPkgManager() === "npm") {
    const nodeModulesPath = path.join(
      projectPath,
      "packages/on-chain-script/node_modules",
    );
    const testtoolLinkPath = path.join(nodeModulesPath, "ckb-testtool");
    const testtoolTargetPath = "node_modules/ckb-testtool";

    try {
      fs.ensureDirSync(nodeModulesPath);
      // Remove existing link/directory if it exists
      if (fs.pathExistsSync(testtoolLinkPath)) {
        fs.removeSync(testtoolLinkPath);
      }
      fs.ensureSymlinkSync(testtoolTargetPath, testtoolLinkPath);
      console.log(
        green(
          `Created symlink: node_modules/ckb-testtool -> ${testtoolTargetPath}`,
        ),
      );
    } catch (error: any) {
      console.warn(
        yellow(
          `Warning: Failed to create ckb-testtool symlink: ${error.message}`,
        ),
      );
    }
  }

  console.log(`${green("Success!")} Created ${projectName} at ${projectPath}`);
  console.log();
}

const update = updateCheck(packageJson).catch(() => null);

async function notifyUpdate(): Promise<void> {
  try {
    const updateInfo = await update;
    if (updateInfo?.latest) {
      const global: Record<string, string> = {
        pnpm: "pnpm add",
        npm: "npm install",
      };
      const updateMessage = `${global[packageManager]} ${packageJson.name}`;
      console.log(
        yellow(bold(`A new version of \`${packageJson.name}\` is available!`)) +
          "\n" +
          "You can update by running: " +
          cyan(updateMessage) +
          "\n",
      );
    }
    process.exit(0);
  } catch {
    // ignore error
  }
}

interface ExitReason {
  command?: string;
  message?: string;
}

async function exit(reason: ExitReason): Promise<void> {
  console.log();
  console.log("Aborting installation.");
  if (reason.command) {
    console.log(`  ${cyan(reason.command)} has failed.`);
  } else {
    console.log(
      red("Unexpected error. Please report it as a bug:") + "\n",
      reason,
    );
  }
  console.log();
  await notifyUpdate();
  process.exit(1);
}

(async () => {
  try {
    await run();
    await notifyUpdate();
  } catch (error) {
    await exit(error as ExitReason);
  }
})();
