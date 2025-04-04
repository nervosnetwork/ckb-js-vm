/** @type {import('ts-jest').JestConfigWithTsJest} */
module.exports = {
  preset: "ts-jest",
  testEnvironment: "node",
  testPathIgnorePatterns: ["/node_modules/", "/dist/", "/dist.commonjs/"],

  transform: {
    "^.+\\.ts$": [
      "ts-jest",
      {
        tsconfig: "./tsconfig.jest.json",
      },
    ],
  },
  maxWorkers: 1,
  verbose: true,
};
